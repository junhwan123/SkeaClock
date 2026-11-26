#pragma once

#include "../language/language.h"

struct MenuFileType {
    const wchar_t* UiStrings::* label;
    const wchar_t* pattern;
};

constexpr MenuFileType LanguageFileTypes[] = {
    { &UiStrings::languageFiles, L"*.h" },
    { &UiStrings::allFiles, L"*.*" },
};
constexpr MenuFileType ProgramFileTypes[] = {
    { &UiStrings::programFiles, L"*.exe" },
    { &UiStrings::allFiles, L"*.*" },
};
constexpr MenuFileType MediaFileTypes[] = {
    { &UiStrings::mediaFiles, L"*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma;*.mp4;*.m4v;*.mov;*.wmv" },
    { &UiStrings::audioFiles, L"*.wav;*.mp3;*.m4a;*.aac;*.flac;*.wma" },
    { &UiStrings::videoFiles, L"*.mp4;*.m4v;*.mov;*.wmv" },
};

template<size_t Count>
std::wstring MenuFileFilter(const MenuFileType (&types)[Count]) {
    std::wstring filter;
    const auto& text = UiText();
    for (const auto& type : types) {
        filter += text.*type.label;
        filter += L'\0';
        filter += type.pattern;
        filter += L'\0';
    }
    return filter;
}
