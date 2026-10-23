$ErrorActionPreference = 'Stop'
$productVersion = '2.1.2'
$lowResolutionScale = '0.6'
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path $PSScriptRoot '.build'
$output = Join-Path $PSScriptRoot 'SkeaClock.exe'
# shortcut: deleting the previous EXE resets the revision; use a state file if numbering must survive deletion.
$revision = 0
if (Test-Path -LiteralPath $output -PathType Leaf) {
    $previousVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($output)
    if ($previousVersion.ProductVersion -eq $productVersion) {
        $revision = $previousVersion.FilePrivatePart + 1
    }
}
$fileVersion = "$productVersion.$revision"
$utf8 = New-Object Text.UTF8Encoding($true)
function Write-Utf8($path, $text) { [IO.File]::WriteAllText($path, $text, $utf8) }
function Replace-Once($text, $old, $new) {
    if ($text.IndexOf($old) -lt 0 -or $text.IndexOf($old) -ne $text.LastIndexOf($old)) {
        throw 'Source changed: review the standalone adapter before building.'
    }
    return $text.Replace($old, $new)
}
function Clear-Work {
    if (Test-Path -LiteralPath $work) {
        $resolved = (Resolve-Path -LiteralPath $work).Path
        if ($resolved -ne ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '.build')))) {
            throw 'Unexpected build directory.'
        }
        if ((Get-Item -LiteralPath $work).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw 'Build directory must not be a link.'
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
Clear-Work
New-Item -ItemType Directory -Path $work | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'skea-clock.cpp') -Destination $work
Get-ChildItem -LiteralPath $root -Filter '*.h' -File | Copy-Item -Destination $work
Copy-Item -LiteralPath (Join-Path $root 'languages') -Destination $work -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'standalone.h') -Destination $work

$source = Join-Path $root 'sources'
& python.exe -B (Join-Path $PSScriptRoot 'prepare-low-resolution.py') $lowResolutionScale
if ($LASTEXITCODE -ne 0) { throw 'Low-resolution image preparation failed.' }
Get-ChildItem -LiteralPath $source -Recurse -File -Filter '*.ico' | ForEach-Object {
    $relative = $_.FullName.Substring($source.Length + 1)
    $target = Join-Path (Join-Path $work 'sources') $relative
    New-Item -ItemType Directory -Force -Path (Split-Path $target -Parent) | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $target
}
Copy-Item -LiteralPath (Join-Path $source 'clock-dial-data.h') -Destination (Join-Path $work 'sources')
$sounds = @('idle1', 'idle2', 'smash', 'touch_start', 'touch_end', 'touch1', 'touch2', 'touch3',
    'pat_start', 'pat_end', 'pat1', 'pat2', 'pat3', 'blank1', 'answer', 'smile', 'alert', 'base_alert')
$cpp = [IO.File]::ReadAllText((Join-Path $work 'skea-clock.cpp')).Replace("`r`n", "`n")
$cpp = Replace-Once $cpp '#include "clock-artwork.h"' "#include `"standalone.h`"`n#include `"clock-artwork.h`""
$old = @'
    wchar_t executable[MAX_PATH];
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    settingsPath = executable;
    settingsPath = settingsPath.substr(0, settingsPath.find_last_of(L"\\/") + 1) + L"skea-clock.ini";
'@
$new = @'
    std::wstring sourceDirectory;
    if (!PrepareStandalone(settingsPath, sourceDirectory)) {
        LoadLanguageSettings(settingsPath);
        MessageBoxW(nullptr, UiText().standaloneDataError, L"Skea clock", MB_OK | MB_ICONERROR);
        CloseHandle(mutex);
        return 1;
    }
'@
$cpp = Replace-Once $cpp ($old.Replace("`r`n", "`n")) ($new.Replace("`r`n", "`n"))
$old = @'
    std::wstring directory = settingsPath.substr(0, settingsPath.find_last_of(L"\\/") + 1);
    std::wstring sourceDirectory = directory + L"sources\\";
    if (GetFileAttributesW((sourceDirectory + L"locate data\\clock-layout.ini").c_str()) == INVALID_FILE_ATTRIBUTES)
        sourceDirectory = directory + L"..\\sources\\";
'@
$cpp = Replace-Once $cpp ($old.Replace("`r`n", "`n")) ''
$cpp = Replace-Once $cpp 'UiText().artworkLoadHelp' 'UiText().standaloneArtworkLoadHelp'
Write-Utf8 (Join-Path $work 'skea-clock.cpp') $cpp
$variants = @(
    @{ File = 'SkeaClock.exe'; Source = $source; Scale = '1.0' },
    @{ File = 'SkeaClock_low.exe'; Source = (Join-Path $source 'low-resolution'); Scale = $lowResolutionScale }
)
$outputs = @()
foreach ($variant in $variants) {
    $layout = Join-Path $variant.Source 'locate data\clock-layout.ini'
    $paths = @('locate data\clock-layout.ini') + @(Get-Content -LiteralPath $layout -Encoding Unicode | Where-Object {
        $_ -match '^file\d*='
    } | ForEach-Object { $_.Substring($_.IndexOf('=') + 1) })
    $paths += @($sounds | ForEach-Object { "sound\$_.wav" })
    $rc = @('#pragma code_page(65001)') + @(Get-Content -LiteralPath (Join-Path $root 'skea-clock.rc') -Encoding UTF8)
    $table = @('struct EmbeddedAsset { int id; const wchar_t* path; };', 'constexpr EmbeddedAsset EmbeddedAssets[] = {')
    $id = 100
    foreach ($relative in ($paths | Select-Object -Unique)) {
        if ([IO.Path]::IsPathRooted($relative) -or $relative.Split('\') -contains '..') { throw "Invalid asset path: $relative" }
        $assetRoot = if ($relative.StartsWith('sound\')) { $source } else { $variant.Source }
        $input = Join-Path $assetRoot $relative
        if (!(Test-Path -LiteralPath $input -PathType Leaf)) { throw "Missing asset: $input" }
        $rc += "$id RCDATA `"$($input.Replace('\', '\\'))`""
        $table += "    { $id, L`"$($relative.Replace('\', '\\'))`" },"
        $id++
    }
    $table += '};'
    Write-Utf8 (Join-Path $work 'assets.rc') ($rc -join "`r`n")
    Write-Utf8 (Join-Path $work 'assets.h') ($table -join "`r`n")
    $output = Join-Path $PSScriptRoot $variant.File
    Push-Location -LiteralPath $work
    try {
        & rc.exe /nologo /fo assets.res assets.rc
        if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
        & cl.exe /nologo /std:c++17 /W4 /WX /O2 /MT /EHsc /utf-8 /D_WIN32_WINNT=0x0A00 "/DSKEA_IMAGE_SCALE=$($variant.Scale)f" skea-clock.cpp assets.res /Fo:clock.obj /Fe:SkeaClock.exe /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED
        if ($LASTEXITCODE -ne 0) { throw 'C++ compilation failed.' }
        & (Join-Path $PSScriptRoot 'update-metadata.ps1') -ExePath (Join-Path $work 'SkeaClock.exe') -ProductVersion $productVersion -FileVersion $fileVersion
        Copy-Item -LiteralPath (Join-Path $work 'SkeaClock.exe') -Destination $output -Force
    } finally { Pop-Location }
    $outputs += $output
}
Set-Location -LiteralPath $PSScriptRoot
Clear-Work
& python.exe -B (Join-Path $root 'language-editor\generate.py')
if ($LASTEXITCODE -ne 0) { throw 'Language editor generation failed.' }
Get-Item -LiteralPath $outputs | Select-Object Name, Length
