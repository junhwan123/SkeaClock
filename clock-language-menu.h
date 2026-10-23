#pragma once
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

constexpr int LanguageFileChoose = 4998, LanguageCustom = 4999;
constexpr int LanguageFirst = 5000;
static_assert(std::size(LanguagePacks) <= 0x10000 - LanguageFirst,
    "Too many languages for menu command IDs.");

std::wstring customLanguagePath;

void SaveLanguageSettings(const std::wstring& path) {
    WritePrivateProfileStringW(L"clock", L"language", selectedLanguage.c_str(), path.c_str());
    WritePrivateProfileStringW(L"clock", L"languageFile", customLanguagePath.c_str(), path.c_str());
    WritePrivateProfileStringW(L"clock", L"useCustomLanguage", useCustomLanguage ? L"1" : L"0", path.c_str());
}

void ShowLanguageFileError(HWND owner, const std::wstring& path) {
    const std::wstring message = std::wstring(UiText().languageFileError) + L"\n\n" + path;
    MessageBoxW(owner, message.c_str(), L"Skea clock", MB_OK | MB_ICONWARNING);
}

bool ReloadCustomLanguage(HWND owner, const std::wstring& settings) {
    if (customLanguagePath.empty()) return false;
    auto language = ReadCustomLanguage(customLanguagePath);
    if (!language) {
        const std::wstring failedPath = customLanguagePath;
        customLanguagePath.clear();
        customLanguage.reset();
        if (useCustomLanguage) selectedLanguage = L"ko";
        useCustomLanguage = false;
        SaveLanguageSettings(settings);
        ShowLanguageFileError(owner, failedPath);
        return false;
    }
    customLanguage = std::move(language);
    if (useCustomLanguage) selectedLanguage = customLanguage->strings.languageCode;
    return true;
}

void LoadLanguageSettings(const std::wstring& path) {
    wchar_t language[16] = {};
    GetPrivateProfileStringW(L"clock", L"language", L"ko", language, 16, path.c_str());
    selectedLanguage = L"ko";
    for (const auto& pack : LanguagePacks) {
        if (wcscmp(language, pack.code) != 0) continue;
        selectedLanguage = pack.code;
        break;
    }
    wchar_t file[32768] = {};
    GetPrivateProfileStringW(L"clock", L"languageFile", L"", file, static_cast<DWORD>(std::size(file)), path.c_str());
    customLanguagePath = file;
    useCustomLanguage = !customLanguagePath.empty() && GetPrivateProfileIntW(L"clock", L"useCustomLanguage", 0, path.c_str()) != 0;
    ReloadCustomLanguage(nullptr, path);
}

bool ChooseLanguageFile(HWND owner) {
    wchar_t file[32768] = {};
    OPENFILENAMEW dialog = { sizeof(OPENFILENAMEW) };
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = UiText().languageFileFilter;
    dialog.lpstrFile = file;
    dialog.nMaxFile = static_cast<DWORD>(std::size(file));
    dialog.lpstrTitle = UiText().languageFileTitle;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) return false;
    auto language = ReadCustomLanguage(file);
    if (!language) {
        ShowLanguageFileError(owner, file);
        return false;
    }
    customLanguagePath = file;
    customLanguage = std::move(language);
    selectedLanguage = customLanguage->strings.languageCode;
    useCustomLanguage = true;
    return true;
}

HMENU CreateLanguageMenu() {
    HMENU menu = CreatePopupMenu();
    int command = LanguageFirst;
    for (const auto& language : LanguagePacks) {
        AppendMenuW(menu, MF_STRING | (!useCustomLanguage && selectedLanguage == language.code ? MF_CHECKED : 0),
            command++, language.nativeName);
    }
    if (customLanguage)
        AppendMenuW(menu, MF_STRING | (useCustomLanguage ? MF_CHECKED : 0), LanguageCustom, customLanguage->strings.nativeName);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, LanguageFileChoose, UiText().languageFileChoose);
    return menu;
}
