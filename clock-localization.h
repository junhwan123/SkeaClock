#pragma once
#include <array>
#include <iterator>

struct UiStrings {
    const wchar_t* languageCode;
    const wchar_t* nativeName;
    const wchar_t* languageFileChoose;
    const wchar_t* languageFileTitle;
    const wchar_t* languageFileFilter;
    const wchar_t* languageFileError;
    const wchar_t* background;
    const wchar_t* ticks;
    const wchar_t* numbers;
    const wchar_t* alarmAdd;
    const wchar_t* alarmEdit;
    const wchar_t* alarmDelete;
    const wchar_t* effectVolume;
    const wchar_t* alarmFileChoose;
    const wchar_t* alarmDefault;
    const wchar_t* volume;
    const wchar_t* alarmSound;
    const wchar_t* alwaysOnTop;
    const wchar_t* topmostExceptions;
    const wchar_t* noPrograms;
    const wchar_t* programChoose;
    const wchar_t* runAtStartup;
    const wchar_t* startupError;
    const wchar_t* taskbar;
    const wchar_t* layout;
    const wchar_t* displaySelect;
    const wchar_t* displayPrefix;
    const wchar_t* displayPrimary;
    const wchar_t* exit;
    const wchar_t* autoUpdateCheck;
    const wchar_t* githubVisit;
    const wchar_t* updateAvailable;
    const wchar_t* updateVisitGithub;
    const wchar_t* updateIgnore;
    const wchar_t* updateCancel;
    std::array<const wchar_t*, 7> alarmDays;
    const wchar_t* alarmNoDays;
    const wchar_t* alarmFileFilter;
    const wchar_t* alarmFileTitle;
    const wchar_t* programFileFilter;
    const wchar_t* programFileTitle;
    const wchar_t* mediaPackMissing;
    const wchar_t* alarmFileError;
    const wchar_t* artworkLoadError;
    const wchar_t* artworkLoadHelp;
    const wchar_t* standaloneArtworkLoadHelp;
    const wchar_t* clockWindowError;
    const wchar_t* standaloneDataError;
};

#include "languages/ko.h"
#include "languages/ja.h"
#include "languages/en.h"
#include "languages/zh-CN.h"

struct LanguagePack {
    const wchar_t* code;
    const wchar_t* nativeName;
    const UiStrings& strings;
};

const LanguagePack LanguagePacks[] = {
    { KoreanStrings.languageCode, KoreanStrings.nativeName, KoreanStrings },
    { JapaneseStrings.languageCode, JapaneseStrings.nativeName, JapaneseStrings },
    { EnglishStrings.languageCode, EnglishStrings.nativeName, EnglishStrings },
    { SimplifiedChineseStrings.languageCode, SimplifiedChineseStrings.nativeName, SimplifiedChineseStrings },
};
std::wstring selectedLanguage = L"ko";

#include "clock-custom-language.h"

const UiStrings& UiText() {
    if (useCustomLanguage && customLanguage) return customLanguage->strings;
    for (const auto& language : LanguagePacks)
        if (selectedLanguage == language.code) return language.strings;
    return KoreanStrings;
}
