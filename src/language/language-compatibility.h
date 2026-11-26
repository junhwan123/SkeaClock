#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <vector>

using LanguageValues = std::map<std::wstring, std::vector<std::wstring>>;

bool ImportLegacyFilter(LanguageValues& values, const wchar_t* source,
    std::initializer_list<const wchar_t*> labels) {
    const auto found = values.find(source);
    if (found == values.end()) return true;
    if (found->second.size() != 1) return false;
    const auto& filter = found->second.front();
    if (filter.empty()) return true;
    if (filter.size() < 2 || filter[filter.size() - 1] || filter[filter.size() - 2]) return false;
    auto label = labels.begin();
    for (size_t position = 0; position < filter.size() - 1;) {
        const auto end = filter.find(L'\0', position);
        if (end == std::wstring::npos) return false;
        const auto patternEnd = filter.find(L'\0', end + 1);
        if (patternEnd == std::wstring::npos || patternEnd == end + 1) return false;
        if (label != labels.end()) {
            if (end != position) values.try_emplace(*label, std::vector{ filter.substr(position, end - position) });
            ++label;
        }
        position = patternEnd + 1;
    }
    return true;
}

bool ImportLegacyFileTypes(LanguageValues& values) {
    return ImportLegacyFilter(values, L"languageFileFilter", { L"languageFiles", L"allFiles" }) &&
        ImportLegacyFilter(values, L"alarmFileFilter", { L"mediaFiles", L"audioFiles", L"videoFiles" }) &&
        ImportLegacyFilter(values, L"programFileFilter", { L"programFiles", L"allFiles" });
}

std::wstring* LegacyLanguageText(LanguageValues& values, const wchar_t* key) {
    const auto found = values.find(key);
    return found != values.end() && found->second.size() == 1 ? &found->second.front() : nullptr;
}

void TrimLanguageLabelEnd(std::wstring& text, const wchar_t* characters) {
    const auto last = text.find_last_not_of(characters);
    if (last == std::wstring::npos) text.clear();
    else text.erase(last + 1);
}

void ImportLegacyLanguagePhrases(LanguageValues& values) {
    constexpr const wchar_t* space = L" \t\r\n";
    for (const auto* key : { L"languageFileChoose", L"alarmFileChoose", L"musicFileChoose", L"programChoose" }) {
        if (auto* phrase = LegacyLanguageText(values, key)) TrimLanguageLabelEnd(*phrase, L" \t\r\n.…");
    }
    if (auto* prefix = LegacyLanguageText(values, L"displayPrefix")) TrimLanguageLabelEnd(*prefix, space);
    if (auto* primary = LegacyLanguageText(values, L"displayPrimary"))
        primary->erase(0, primary->find_first_not_of(L" \t\r\n·・"));
    if (auto* empty = LegacyLanguageText(values, L"noPrograms"); empty && empty->size() >= 2) {
        if ((empty->front() == L'(' && empty->back() == L')') ||
            (empty->front() == L'（' && empty->back() == L'）'))
            *empty = empty->substr(1, empty->size() - 2);
    }

    constexpr const wchar_t* pairs[][2] = {
        { L"updateAvailable", L"updateDownloadQuestion" },
        { L"standaloneDataError", L"standaloneDataHelp" },
        { L"artworkLoadError", L"artworkRecoveryHelp" },
        { L"languageFileError", L"languageFileHelp" },
        { L"startupError", L"startupHelp" },
    };
    for (const auto& pair : pairs) {
        auto* message = LegacyLanguageText(values, pair[0]);
        if (!message || values.count(pair[1])) continue;
        constexpr const wchar_t* ends = L".!?。！？";
        for (auto end = message->find_first_of(ends); end != std::wstring::npos;
            end = message->find_first_of(ends, end + 1)) {
            const auto start = message->find_first_not_of(L" \t\r\n.!?。！？", end + 1);
            if (start == std::wstring::npos) break;
            if ((*message)[end] < 128 && start == end + 1) continue;
            values.emplace(pair[1], std::vector{ message->substr(start) });
            message->erase(end);
            break;
        }
    }
}
