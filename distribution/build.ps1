$ErrorActionPreference = 'Stop'
$productVersion = '3.3.2'
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
if ($revision -gt 65535) { throw 'File revision exceeds 65535. Set a new product version.' }
$utf8 = New-Object Text.UTF8Encoding($true)
function Write-Utf8($path, $text) { [IO.File]::WriteAllText($path, $text, $utf8) }
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
Copy-Item -LiteralPath (Join-Path $root 'src') -Destination $work -Recurse
$languageWork = Join-Path $work 'languages'
New-Item -ItemType Directory -Path $languageWork | Out-Null
foreach ($name in @('ko.h', 'ja.h', 'en.h', 'zh-CN.h')) {
    Copy-Item -LiteralPath (Join-Path $root "languages\$name") -Destination $languageWork
}

$source = Join-Path $root 'sources'
Get-ChildItem -LiteralPath $source -Recurse -File -Filter '*.ico' | ForEach-Object {
    $relative = $_.FullName.Substring($source.Length + 1)
    $target = Join-Path (Join-Path $work 'sources') $relative
    New-Item -ItemType Directory -Force -Path (Split-Path $target -Parent) | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $target
}
$commonSounds = @('pat_start', 'pat_end', 'touch_start', 'touch_end', 'base_alert')
$sounds = @('idle1', 'idle2', 'smash', 'touch1', 'touch2', 'touch3',
    'pat1', 'pat2', 'pat3', 'blank1', 'answer', 'smile', 'alert')
$muteSounds = @('idle', 'smash', 'blank', 'answer', 'smile', 'alert',
    'pat1', 'pat2', 'pat3', 'touch1', 'touch2', 'touch3')
$kommySounds = @('idle', 'smash', 'blank', 'answer', 'alert', 'dial',
    'pat1', 'pat2', 'pat3', 'touch1', 'touch2', 'touch3')
$coffeSounds = @('idle1', 'idle2', 'smash', 'blank', 'answer',
    'pat1', 'pat2', 'pat3', 'touch1', 'touch2', 'touch3',
    'coffee1 start', 'coffee1 idle1', 'coffee1 idle2', 'coffee2 start', 'coffee3 start', 'coffee end1', 'drinking')
$coffeProps = @('table', 'coffee-bag', 'coffee-pot', 'coffee-cup-blank', 'coffee-cup-filled')
$variants = @(
    @{ File = 'SkeaClock.exe'; Source = $source; Scale = '1.0' },
    @{ File = 'SkeaClock_low.exe'; Source = (Join-Path $source 'low-resolution'); Scale = $lowResolutionScale }
)
$layouts = @(
    'characters\Skea\locate data\skea-layout.ini',
    'characters\Mute\locate data\mute-layout.ini',
    'characters\Kommy\locate data\kommy-layout.ini',
    'characters\Coffe\locate data\coffe-layout.ini'
)
foreach ($variant in $variants) {
    $paths = @()
    foreach ($relative in $layouts) {
        $characterPath = Split-Path -Parent (Split-Path -Parent $relative)
        $paths += $relative
        $paths += @(Get-Content -LiteralPath (Join-Path $variant.Source $relative) -Encoding Unicode | Where-Object {
            $_ -match '^file\d*='
        } | ForEach-Object { "$characterPath\$($_.Substring($_.IndexOf('=') + 1))" })
    }
    $paths += @($commonSounds | ForEach-Object { "common\sound\$_.wav" })
    $paths += @($sounds | ForEach-Object { "characters\Skea\sound\$_.wav" })
    $paths += @($muteSounds | ForEach-Object { "characters\Mute\sound\$_.wav" })
    $paths += @($kommySounds | ForEach-Object { "characters\Kommy\sound\$_.wav" })
    $paths += @($coffeSounds | ForEach-Object { "characters\Coffe\sound\$_.wav" })
    $paths += @($coffeProps | ForEach-Object { "characters\Coffe\$_.png" })
    $rc = @(
        '#pragma code_page(65001)'
        "#define SKEA_PRODUCT_VERSION $($productVersion.Replace('.', ',')),0"
        "#define SKEA_FILE_VERSION $($fileVersion.Replace('.', ','))"
        ('#define SKEA_PRODUCT_VERSION_TEXT "{0}"' -f $productVersion)
        ('#define SKEA_FILE_VERSION_TEXT "{0}"' -f $fileVersion)
        ('#define SKEA_ORIGINAL_FILENAME "{0}"' -f $variant.File)
        ('#define SKEA_INTERNAL_NAME "{0}"' -f ([IO.Path]::GetFileNameWithoutExtension($variant.File)))
    ) + @(Get-Content -LiteralPath (Join-Path $root 'skea-clock.rc') -Encoding UTF8)
    $table = @('struct EmbeddedAsset { int id; const wchar_t* path; };', 'constexpr EmbeddedAsset EmbeddedAssets[] = {')
    $id = 100
    foreach ($relative in ($paths | Select-Object -Unique)) {
        if ([IO.Path]::IsPathRooted($relative) -or $relative.Split('\') -contains '..') { throw "Invalid asset path: $relative" }
        $assetRoot = if ($relative.EndsWith('.wav')) { $source } else { $variant.Source }
        $input = Join-Path $assetRoot $relative
        if (!(Test-Path -LiteralPath $input -PathType Leaf)) { throw "Missing asset: $input" }
        $rc += "$id RCDATA `"$($input.Replace('\', '\\'))`""
        $table += "    { $id, L`"$($relative.Replace('\', '\\'))`" },"
        $id++
    }
    $table += '};'
    Write-Utf8 (Join-Path $work 'assets.rc') ($rc -join "`r`n")
    Write-Utf8 (Join-Path $work 'src\app\assets.h') ($table -join "`r`n")
    $resourceFile = [IO.Path]::ChangeExtension($variant.File, '.res')
    Push-Location -LiteralPath $work
    try {
        & rc.exe /nologo /fo $resourceFile assets.rc
        if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
        & cl.exe /nologo /std:c++17 /W4 /WX /O2 /MT /EHsc /utf-8 /D_WIN32_WINNT=0x0A00 "/DSKEA_IMAGE_SCALE=$($variant.Scale)f" skea-clock.cpp $resourceFile /Fo:clock.obj "/Fe:$($variant.File)" /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED
        if ($LASTEXITCODE -ne 0) { throw 'C++ compilation failed.' }
    } finally { Pop-Location }
}
$outputs = foreach ($variant in $variants) {
    $output = Join-Path $PSScriptRoot $variant.File
    Copy-Item -LiteralPath (Join-Path $work $variant.File) -Destination $output -Force
    $output
}
Set-Location -LiteralPath $PSScriptRoot
Clear-Work
Get-Item -LiteralPath $outputs | Select-Object Name, Length
