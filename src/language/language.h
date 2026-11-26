#pragma once

#include <array>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "language-compatibility.h"

struct UiStrings {
    const wchar_t* languageCode;
    const wchar_t* nativeName;
    const wchar_t* languageFileChoose;
    const wchar_t* languageFileTitle;
    const wchar_t* languageFiles;
    const wchar_t* allFiles;
    const wchar_t* background;
    const wchar_t* timeDisplay;
    const wchar_t* ticks;
    const wchar_t* numbers;
    const wchar_t* hologram;
    const wchar_t* format24Hour;
    const wchar_t* alarmAdd;
    const wchar_t* alarmEdit;
    const wchar_t* alarmDelete;
    const wchar_t* effectVolume;
    const wchar_t* alarmFileChoose;
    const wchar_t* alarmDefault;
    const wchar_t* volume;
    const wchar_t* alarmSound;
    const wchar_t* musicSound;
    const wchar_t* musicFileChoose;
    const wchar_t* musicAutoPlay;
    const wchar_t* tableDisplay;
    const wchar_t* playerDisplay;
    const wchar_t* alwaysOnTop;
    const wchar_t* topmostExceptions;
    const wchar_t* noPrograms;
    const wchar_t* programChoose;
    const wchar_t* runAtStartup;
    const wchar_t* taskbar;
    const wchar_t* layout;
    const wchar_t* displaySelect;
    const wchar_t* displayPrefix;
    const wchar_t* displayPrimary;
    const wchar_t* exit;
    const wchar_t* autoUpdateCheck;
    const wchar_t* githubVisit;
    std::array<const wchar_t*, 7> alarmDays;
    const wchar_t* alarmNoDays;
    const wchar_t* mediaFiles;
    const wchar_t* audioFiles;
    const wchar_t* videoFiles;
    const wchar_t* alarmFileTitle;
    const wchar_t* programFiles;
    const wchar_t* programFileTitle;

    const wchar_t* updateAvailable;
    const wchar_t* updateDownloadQuestion;
    const wchar_t* updateDownload;
    const wchar_t* updateCancel;
    const wchar_t* settingsDowngradeWarning;
    const wchar_t* settingsDowngradeContinue;
    const wchar_t* standaloneDataError;
    const wchar_t* standaloneDataHelp;
    const wchar_t* artworkLoadError;
    const wchar_t* artworkRecoveryHelp;
    const wchar_t* runtimeError;
    const wchar_t* settingsVersionError;
    const wchar_t* languageFileError;
    const wchar_t* languageFileHelp;
    const wchar_t* startupError;
    const wchar_t* startupHelp;
    const wchar_t* alarmFileError;
    const wchar_t* mediaValidationStartError;
    const wchar_t* mediaValidationIncomplete;
    const wchar_t* mediaPackMissing;
    const wchar_t* musicFileError;
};

#include "../../languages/ko.h"

struct LanguageToken {
    std::wstring text;
    bool literal = false;
};

class LanguageLexer {
    const std::wstring& source;
    size_t position = 0;

    static int Hex(wchar_t value) {
        if (value >= L'0' && value <= L'9') return value - L'0';
        if (value >= L'a' && value <= L'f') return value - L'a' + 10;
        if (value >= L'A' && value <= L'F') return value - L'A' + 10;
        return -1;
    }

    bool String(std::wstring& value) {
        ++position;
        while (position < source.size()) {
            wchar_t character = source[position++];
            if (character == L'"') return true;
            if (character == L'\r' || character == L'\n' || character == 0) return false;
            if (character != L'\\') { value += character; continue; }
            if (position == source.size()) return false;
            character = source[position++];
            if (character >= L'0' && character <= L'7') {
                unsigned number = character - L'0';
                for (int count = 1; count < 3 && position < source.size() &&
                    source[position] >= L'0' && source[position] <= L'7'; ++count)
                    number = number * 8 + source[position++] - L'0';
                value += static_cast<wchar_t>(number);
            } else if (character == L'u' || character == L'U' || character == L'x') {
                const size_t count = character == L'u' ? 4 : character == L'U' ? 8 : source.size();
                unsigned number = 0;
                size_t digits = 0;
                while (digits < count && position < source.size() && Hex(source[position]) >= 0) {
                    if (number > 0x10ffff / 16) return false;
                    number = number * 16 + Hex(source[position++]);
                    ++digits;
                }
                if (!digits || (character != L'x' && digits != count) || number > 0x10ffff ||
                    (number >= 0xd800 && number <= 0xdfff)) return false;
                if (number < 0x10000) value += static_cast<wchar_t>(number);
                else {
                    number -= 0x10000;
                    value += static_cast<wchar_t>(0xd800 + (number >> 10));
                    value += static_cast<wchar_t>(0xdc00 + (number & 0x3ff));
                }
            } else {
                switch (character) {
                case L'a': value += L'\a'; break;
                case L'b': value += L'\b'; break;
                case L'f': value += L'\f'; break;
                case L'n': value += L'\n'; break;
                case L'r': value += L'\r'; break;
                case L't': value += L'\t'; break;
                case L'v': value += L'\v'; break;
                case L'\\': case L'"': case L'\'': case L'?': value += character; break;
                default: return false;
                }
            }
        }
        return false;
    }

public:
    explicit LanguageLexer(const std::wstring& text) : source(text) {}

    bool Next(LanguageToken& token) {
        token = {};
        while (position < source.size()) {
            if (std::iswspace(source[position]) || source[position] == 0xfeff) { ++position; continue; }
            if (source.compare(position, 2, L"//") == 0) {
                position = source.find(L'\n', position + 2);
                if (position == std::wstring::npos) position = source.size();
                continue;
            }
            if (source.compare(position, 2, L"/*") == 0) {
                const size_t end = source.find(L"*/", position + 2);
                if (end == std::wstring::npos) return false;
                position = end + 2;
                continue;
            }
            break;
        }
        if (position == source.size()) return true;
        if (source.compare(position, 2, L"L\"") == 0) ++position;
        if (source[position] == L'"') { token.literal = true; return String(token.text); }
        const size_t start = position++;
        const auto identifier = [](wchar_t value) {
            return (value >= L'a' && value <= L'z') || (value >= L'A' && value <= L'Z') || value == L'_';
        };
        if (identifier(source[start]))
            while (position < source.size() && (identifier(source[position]) ||
                (source[position] >= L'0' && source[position] <= L'9'))) ++position;
        token.text = source.substr(start, position - start);
        return true;
    }
};

class LanguageFileParser {
    LanguageLexer lexer;
    LanguageToken token;
    bool valid = true;

    bool Next() { valid = lexer.Next(token); return valid; }
    bool Is(const wchar_t* text) const { return valid && !token.literal && token.text == text; }
    bool Take(const wchar_t* text) { return Is(text) && Next(); }
    bool Identifier(std::wstring& name) {
        if (!valid || token.literal || token.text.empty() ||
            !((token.text[0] >= L'a' && token.text[0] <= L'z') ||
              (token.text[0] >= L'A' && token.text[0] <= L'Z') || token.text[0] == L'_')) return false;
        name = token.text;
        return Next();
    }
    bool String(std::wstring& value) {
        if (!valid || !token.literal) return false;
        do { value += token.text; if (!Next()) return false; } while (token.literal);
        return true;
    }

public:
    explicit LanguageFileParser(const std::wstring& text) : lexer(text) {}

    bool Parse(LanguageValues& values) {
        if (!Next()) return false;
        if (Is(L"#") && !(Take(L"#") && Take(L"pragma") && Take(L"once"))) return false;
        std::wstring name;
        if (!(Take(L"const") && Take(L"UiStrings") && Identifier(name) && Take(L"=") &&
            Take(L"[") && Take(L"]") && Take(L"{") && Take(L"UiStrings") && Take(L"text") && Take(L"="))) return false;
        if (Is(L"{")) { if (!(Take(L"{") && Take(L"}"))) return false; }
        else if (!Take(L"KoreanStrings")) return false;
        if (!Take(L";")) return false;
        while (!Is(L"return")) {
            if (!(Take(L"text") && Take(L".") && Identifier(name) && Take(L"="))) return false;
            std::vector<std::wstring> entries;
            if (Is(L"{")) {
                if (!Take(L"{")) return false;
                while (!Is(L"}")) {
                    std::wstring entry;
                    if (!String(entry)) return false;
                    entries.push_back(std::move(entry));
                    if (!Is(L"}") && !Take(L",")) return false;
                }
                if (!Take(L"}")) return false;
            } else {
                std::wstring entry;
                if (!String(entry)) return false;
                entries.push_back(std::move(entry));
            }
            if (!Take(L";") || !values.emplace(name, std::move(entries)).second) return false;
        }
        return Take(L"return") && Take(L"text") && Take(L";") && Take(L"}") &&
            Take(L"(") && Take(L")") && Take(L";") && Is(L"");
    }
};

bool ReadLanguageValues(const char* bytes, int size, LanguageValues& values) {
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes, size, nullptr, 0);
    if (!length) return false;
    std::wstring source(length, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes, size, source.data(), length)) return false;
    return LanguageFileParser(source).Parse(values);
}

bool ReadLanguageValues(const std::wstring& path, LanguageValues& values) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size = {};
    const bool sized = GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart <= 1024 * 1024;
    std::vector<char> bytes(sized ? static_cast<size_t>(size.QuadPart) : 0);
    DWORD read = 0;
    const bool loaded = sized && ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size();
    CloseHandle(file);
    if (!loaded) return false;
    return ReadLanguageValues(bytes.data(), static_cast<int>(bytes.size()), values);
}

struct LoadedLanguage {
    LanguageValues values;
    UiStrings strings = {};
};

constexpr const wchar_t* LanguageResources[] = { L"LANG_JA", L"LANG_EN", L"LANG_ZH_CN" };
std::array<std::unique_ptr<LoadedLanguage>, 1 + std::size(LanguageResources)> LanguagePacks;
std::unique_ptr<LoadedLanguage> customLanguage;
std::wstring selectedLanguage = L"ko", customLanguagePath;
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
    { L"languageFiles", &UiStrings::languageFiles },
    { L"allFiles", &UiStrings::allFiles },
    { L"background", &UiStrings::background },
    { L"timeDisplay", &UiStrings::timeDisplay },
    { L"ticks", &UiStrings::ticks },
    { L"numbers", &UiStrings::numbers },
    { L"hologram", &UiStrings::hologram },
    { L"format24Hour", &UiStrings::format24Hour },
    { L"alarmAdd", &UiStrings::alarmAdd },
    { L"alarmEdit", &UiStrings::alarmEdit },
    { L"alarmDelete", &UiStrings::alarmDelete },
    { L"effectVolume", &UiStrings::effectVolume },
    { L"alarmFileChoose", &UiStrings::alarmFileChoose },
    { L"alarmDefault", &UiStrings::alarmDefault },
    { L"volume", &UiStrings::volume },
    { L"alarmSound", &UiStrings::alarmSound },
    { L"musicSound", &UiStrings::musicSound },
    { L"musicFileChoose", &UiStrings::musicFileChoose },
    { L"musicAutoPlay", &UiStrings::musicAutoPlay },
    { L"tableDisplay", &UiStrings::tableDisplay },
    { L"playerDisplay", &UiStrings::playerDisplay },
    { L"alwaysOnTop", &UiStrings::alwaysOnTop },
    { L"topmostExceptions", &UiStrings::topmostExceptions },
    { L"noPrograms", &UiStrings::noPrograms },
    { L"programChoose", &UiStrings::programChoose },
    { L"runAtStartup", &UiStrings::runAtStartup },
    { L"taskbar", &UiStrings::taskbar },
    { L"layout", &UiStrings::layout },
    { L"displaySelect", &UiStrings::displaySelect },
    { L"displayPrefix", &UiStrings::displayPrefix },
    { L"displayPrimary", &UiStrings::displayPrimary },
    { L"exit", &UiStrings::exit },
    { L"autoUpdateCheck", &UiStrings::autoUpdateCheck },
    { L"githubVisit", &UiStrings::githubVisit },
    { L"alarmNoDays", &UiStrings::alarmNoDays },
    { L"mediaFiles", &UiStrings::mediaFiles },
    { L"audioFiles", &UiStrings::audioFiles },
    { L"videoFiles", &UiStrings::videoFiles },
    { L"alarmFileTitle", &UiStrings::alarmFileTitle },
    { L"programFiles", &UiStrings::programFiles },
    { L"programFileTitle", &UiStrings::programFileTitle },

    { L"updateAvailable", &UiStrings::updateAvailable },
    { L"updateDownloadQuestion", &UiStrings::updateDownloadQuestion },
    { L"updateDownload", &UiStrings::updateDownload },
    { L"updateCancel", &UiStrings::updateCancel },
    { L"settingsDowngradeWarning", &UiStrings::settingsDowngradeWarning },
    { L"settingsDowngradeContinue", &UiStrings::settingsDowngradeContinue },
    { L"standaloneDataError", &UiStrings::standaloneDataError },
    { L"standaloneDataHelp", &UiStrings::standaloneDataHelp },
    { L"artworkLoadError", &UiStrings::artworkLoadError },
    { L"artworkRecoveryHelp", &UiStrings::artworkRecoveryHelp },
    { L"runtimeError", &UiStrings::runtimeError },
    { L"settingsVersionError", &UiStrings::settingsVersionError },
    { L"languageFileError", &UiStrings::languageFileError },
    { L"languageFileHelp", &UiStrings::languageFileHelp },
    { L"startupError", &UiStrings::startupError },
    { L"startupHelp", &UiStrings::startupHelp },
    { L"alarmFileError", &UiStrings::alarmFileError },
    { L"mediaValidationStartError", &UiStrings::mediaValidationStartError },
    { L"mediaValidationIncomplete", &UiStrings::mediaValidationIncomplete },
    { L"mediaPackMissing", &UiStrings::mediaPackMissing },
    { L"musicFileError", &UiStrings::musicFileError },
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

std::unique_ptr<LoadedLanguage> MakeLanguage(LanguageValues values) {
    if (!ImportLegacyFileTypes(values)) return nullptr;
    ImportLegacyLanguagePhrases(values);
    auto language = std::make_unique<LoadedLanguage>();
    language->values = std::move(values);
    language->strings = KoreanStrings;
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
        if (value.find(L'\0') != std::wstring::npos) return nullptr;
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

std::unique_ptr<LoadedLanguage> ReadLanguageFile(const std::wstring& path) {
    LanguageValues values;
    if (!ReadLanguageValues(path, values)) return nullptr;
    return MakeLanguage(std::move(values));
}

void LoadBuiltInLanguages() {
    LanguagePacks[0] = std::make_unique<LoadedLanguage>();
    LanguagePacks[0]->strings = KoreanStrings;

    for (size_t i = 0; i < std::size(LanguageResources); ++i) {
        HRSRC resource = FindResourceW(nullptr, LanguageResources[i], RT_RCDATA);
        if (!resource) continue;
        const DWORD size = SizeofResource(nullptr, resource);
        const auto* bytes = static_cast<const char*>(LockResource(LoadResource(nullptr, resource)));
        LanguageValues values;
        if (bytes && size && ReadLanguageValues(bytes, static_cast<int>(size), values))
            LanguagePacks[i + 1] = MakeLanguage(std::move(values));
    }
}

const UiStrings& UiText() {
    if (useCustomLanguage && customLanguage) return customLanguage->strings;
    for (const auto& language : LanguagePacks)
        if (language && selectedLanguage == language->strings.languageCode) return language->strings;
    return LanguagePacks[0] ? LanguagePacks[0]->strings : KoreanStrings;
}

