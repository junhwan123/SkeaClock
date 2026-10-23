#pragma once
#include <cwctype>
#include <string>

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
