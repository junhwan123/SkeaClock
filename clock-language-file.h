#pragma once
#include <map>
#include <utility>
#include <vector>
#include "clock-language-lexer.h"

using LanguageValues = std::map<std::wstring, std::vector<std::wstring>>;

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
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (!length) return false;
    std::wstring source(length, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), source.data(), length)) return false;
    return LanguageFileParser(source).Parse(values);
}
