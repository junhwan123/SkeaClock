#pragma once
#include <memory>
#include "clock-language-file.h"

struct CustomLanguage {
    LanguageValues values;
    UiStrings strings = KoreanStrings;
};

std::unique_ptr<CustomLanguage> customLanguage;
bool useCustomLanguage = false;

struct LanguageStringField {
    const wchar_t* name;
    const wchar_t* UiStrings::* member;
};

const LanguageStringField LanguageStringFields[] = {
    { L"languageCode", &UiStrings::languageCode },
    { L"nativeName", &UiStrings::nativeName },
    { L"languageFileChoose", &UiStrings::languageFileChoose },
    { L"languageFileTitle", &UiStrings::languageFileTitle },
    { L"languageFileFilter", &UiStrings::languageFileFilter },
    { L"languageFileError", &UiStrings::languageFileError },
    { L"background", &UiStrings::background },
    { L"ticks", &UiStrings::ticks },
    { L"numbers", &UiStrings::numbers },
    { L"alarmAdd", &UiStrings::alarmAdd },
    { L"alarmEdit", &UiStrings::alarmEdit },
    { L"alarmDelete", &UiStrings::alarmDelete },
    { L"effectVolume", &UiStrings::effectVolume },
    { L"alarmFileChoose", &UiStrings::alarmFileChoose },
    { L"alarmDefault", &UiStrings::alarmDefault },
    { L"volume", &UiStrings::volume },
    { L"alarmSound", &UiStrings::alarmSound },
    { L"alwaysOnTop", &UiStrings::alwaysOnTop },
    { L"topmostExceptions", &UiStrings::topmostExceptions },
    { L"noPrograms", &UiStrings::noPrograms },
    { L"programChoose", &UiStrings::programChoose },
    { L"runAtStartup", &UiStrings::runAtStartup },
    { L"startupError", &UiStrings::startupError },
    { L"taskbar", &UiStrings::taskbar },
    { L"layout", &UiStrings::layout },
    { L"displaySelect", &UiStrings::displaySelect },
    { L"displayPrefix", &UiStrings::displayPrefix },
    { L"displayPrimary", &UiStrings::displayPrimary },
    { L"exit", &UiStrings::exit },
    { L"autoUpdateCheck", &UiStrings::autoUpdateCheck },
    { L"githubVisit", &UiStrings::githubVisit },
    { L"updateAvailable", &UiStrings::updateAvailable },
    { L"updateVisitGithub", &UiStrings::updateVisitGithub },
    { L"updateIgnore", &UiStrings::updateIgnore },
    { L"updateCancel", &UiStrings::updateCancel },
    { L"alarmNoDays", &UiStrings::alarmNoDays },
    { L"alarmFileFilter", &UiStrings::alarmFileFilter },
    { L"alarmFileTitle", &UiStrings::alarmFileTitle },
    { L"programFileFilter", &UiStrings::programFileFilter },
    { L"programFileTitle", &UiStrings::programFileTitle },
    { L"mediaPackMissing", &UiStrings::mediaPackMissing },
    { L"alarmFileError", &UiStrings::alarmFileError },
    { L"artworkLoadError", &UiStrings::artworkLoadError },
    { L"artworkLoadHelp", &UiStrings::artworkLoadHelp },
    { L"standaloneArtworkLoadHelp", &UiStrings::standaloneArtworkLoadHelp },
    { L"clockWindowError", &UiStrings::clockWindowError },
    { L"standaloneDataError", &UiStrings::standaloneDataError },
};

bool ValidLanguageCode(const std::wstring& code) {
    if (code.empty() || code.size() > 15) return false;
    bool first = true;
    size_t count = 0;
    for (wchar_t character : code) {
        if (character == L'-') {
            if (count < (first ? 2u : 1u) || count > 8) return false;
            first = false;
            count = 0;
        } else {
            const bool letter = (character >= L'a' && character <= L'z') || (character >= L'A' && character <= L'Z');
            if (!letter && (first || character < L'0' || character > L'9')) return false;
            ++count;
        }
    }
    return count >= (first ? 2u : 1u) && count <= 8;
}

bool FillLanguageFilter(std::wstring& value, const wchar_t* baseline) {
    std::vector<std::wstring> labels;
    if (!value.empty()) {
        if (value.size() < 2 || value[value.size() - 1] || value[value.size() - 2]) return false;
        for (size_t position = 0; position < value.size() - 1;) {
            const size_t labelEnd = value.find(L'\0', position);
            const size_t patternEnd = labelEnd == std::wstring::npos ? labelEnd : value.find(L'\0', labelEnd + 1);
            if (patternEnd == std::wstring::npos || patternEnd == labelEnd + 1) return false;
            labels.push_back(value.substr(position, labelEnd - position));
            position = patternEnd + 1;
        }
    }
    value.clear();
    size_t index = 0;
    for (const wchar_t* entry = baseline; *entry; ++index) {
        const std::wstring label = entry;
        entry += label.size() + 1;
        const std::wstring pattern = entry;
        entry += pattern.size() + 1;
        value += index < labels.size() && !labels[index].empty() ? labels[index] : label;
        value += L'\0';
        value += pattern;
        value += L'\0';
    }
    value += L'\0';
    return true;
}

std::unique_ptr<CustomLanguage> ReadCustomLanguage(const std::wstring& path) {
    auto language = std::make_unique<CustomLanguage>();
    if (!ReadLanguageValues(path, language->values)) return nullptr;
    const auto single = [&](const wchar_t* name) -> const std::wstring* {
        const auto found = language->values.find(name);
        return found != language->values.end() && found->second.size() == 1 ? &found->second.front() : nullptr;
    };
    const auto code = single(L"languageCode"), name = single(L"nativeName");
    if (!code || !ValidLanguageCode(*code) || !name || name->size() > 80 ||
        name->find_first_not_of(L" \t\r\n") == std::wstring::npos) return nullptr;
    for (wchar_t character : *name) if (character < L' ' || character == 0x7f) return nullptr;
    for (const auto& field : LanguageStringFields) {
        const auto found = language->values.find(field.name);
        if (found == language->values.end()) continue;
        if (found->second.size() != 1) return nullptr;
        auto& value = found->second.front();
        const bool filter = field.member == &UiStrings::alarmFileFilter || field.member == &UiStrings::programFileFilter ||
            field.member == &UiStrings::languageFileFilter;
        if (filter) {
            if (!FillLanguageFilter(value, KoreanStrings.*field.member)) return nullptr;
        } else if (value.find(L'\0') != std::wstring::npos) return nullptr;
        if (!value.empty()) language->strings.*field.member = value.c_str();
    }
    const auto days = language->values.find(L"alarmDays");
    if (days != language->values.end()) {
        if (days->second.size() > language->strings.alarmDays.size()) return nullptr;
        for (size_t index = 0; index < days->second.size(); ++index) {
            const auto& day = days->second[index];
            if (day.size() > 3 || day.find(L'\0') != std::wstring::npos) return nullptr;
            if (!day.empty()) language->strings.alarmDays[index] = day.c_str();
        }
    }
    return language;
}
