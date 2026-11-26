#pragma once

#include <climits>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <sstream>
#include <vector>

constexpr int MinimumLayoutPercent = 50;
constexpr int MaximumLayoutPercent = 200;
constexpr wchar_t DisplayDataSection[] = L"displaydata";
constexpr wchar_t LegacyLayoutPrefix[] = L"layout_";

std::wstring DisplayLayoutKey(const std::wstring& display) {
    std::wstring key;
    for (wchar_t character : display)
        if (iswalnum(character)) key += character;
    return key;
}

bool ReadDisplayLayoutEntry(const std::wstring& key, RECT& bounds, int& percent) {
    wchar_t value[64] = {};
    const DWORD length = GetPrivateProfileStringW(DisplayDataSection, key.c_str(), L"", value,
        static_cast<DWORD>(std::size(value)), settingsPath.c_str());
    if (!length || length == std::size(value) - 1) return false;

    std::wistringstream entry(value);
    LONG x, y;
    int savedPercent;
    wchar_t firstComma, secondComma;
    if (!(entry >> x >> firstComma >> y >> secondComma >> savedPercent) ||
        firstComma != L',' || secondComma != L',' || !(entry >> std::ws).eof() ||
        savedPercent < MinimumLayoutPercent || savedPercent > MaximumLayoutPercent) return false;
    bounds.left = x;
    bounds.top = y;
    percent = savedPercent;
    return true;
}

bool ReadLegacyDisplayLayout(const std::wstring& section, RECT& bounds, int& percent) {
    constexpr int Missing = INT_MIN;
    const int x = GetPrivateProfileIntW(section.c_str(), L"x", Missing, settingsPath.c_str());
    const int y = GetPrivateProfileIntW(section.c_str(), L"y", Missing, settingsPath.c_str());
    const int savedPercent = GetPrivateProfileIntW(section.c_str(), L"size", Missing, settingsPath.c_str());
    if (x == Missing || y == Missing || savedPercent < MinimumLayoutPercent || savedPercent > MaximumLayoutPercent)
        return false;
    bounds.left = x;
    bounds.top = y;
    percent = savedPercent;
    return true;
}

bool ReadDisplayLayout(const std::wstring& display, RECT& bounds, int& percent) {
    const std::wstring key = DisplayLayoutKey(display);
    return ReadDisplayLayoutEntry(key, bounds, percent) ||
        ReadLegacyDisplayLayout(LegacyLayoutPrefix + key, bounds, percent);
}

bool WriteDisplayLayout(const std::wstring& display, const RECT& bounds, int percent) {
    const std::wstring key = DisplayLayoutKey(display);
    const std::wstring value = std::to_wstring(bounds.left) + L"," +
        std::to_wstring(bounds.top) + L"," + std::to_wstring(percent);
    return WritePrivateProfileStringW(DisplayDataSection, key.c_str(), value.c_str(), settingsPath.c_str()) != FALSE;
}
