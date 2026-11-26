#pragma once

#include <filesystem>
#include "../window/window-layout-settings.h"
#include "error.h"
#include "update.h"

constexpr wchar_t SettingsVersionKey[] = L"currentVersion";
constexpr wchar_t MissingSettingValue[] = L"\x01";

bool HasIniSetting(const std::wstring& path, const wchar_t* section, const wchar_t* key) {
    wchar_t value[2] = {};
    GetPrivateProfileStringW(section, key, MissingSettingValue, value,
        static_cast<DWORD>(std::size(value)), path.c_str());
    return value[0] != MissingSettingValue[0];
}

bool MoveSettingSection(const wchar_t* previous, const wchar_t* current, const wchar_t* key) {
    wchar_t value[32768] = {};
    const DWORD length = GetPrivateProfileStringW(previous, key, MissingSettingValue, value,
        static_cast<DWORD>(std::size(value)), settingsPath.c_str());
    if (value[0] == MissingSettingValue[0]) return true;

    if (!HasIniSetting(settingsPath, current, key) && (length == std::size(value) - 1 ||
        !WritePrivateProfileStringW(current, key, value, settingsPath.c_str()))) return false;

    return WritePrivateProfileStringW(previous, key, nullptr, settingsPath.c_str()) != FALSE;
}

bool MigrateDisplayLayouts() {
    std::vector<wchar_t> sections(256);
    while (GetPrivateProfileSectionNamesW(sections.data(), static_cast<DWORD>(sections.size()),
        settingsPath.c_str()) == sections.size() - 2) {
        sections.resize(sections.size() * 2);
    }

    constexpr size_t PrefixLength = std::size(LegacyLayoutPrefix) - 1;
    for (const wchar_t* section = sections.data(); *section; section += std::wcslen(section) + 1) {
        if (_wcsnicmp(section, LegacyLayoutPrefix, PrefixLength) != 0) continue;
        const std::wstring key = section + PrefixLength;
        if (key.empty() || key != DisplayLayoutKey(key)) continue;
        RECT bounds = {};
        int percent;
        if (!ReadDisplayLayoutEntry(key, bounds, percent)) {
            if (!ReadLegacyDisplayLayout(section, bounds, percent)) continue;
            if (!WriteDisplayLayout(key, bounds, percent)) return false;
        }
        if (!WritePrivateProfileStringW(section, nullptr, nullptr, settingsPath.c_str())) return false;
    }
    return true;
}

bool MigrateSettingsLayout() {
    if (!MigrateDisplayLayouts()) return false;
    for (const wchar_t* key : { L"background", L"ticks", L"numbers" })
        if (!MoveSettingSection(L"clock", L"skea", key)) return false;

    wchar_t legacyKeys[4] = {};
    const bool hadLegacySound = GetPrivateProfileStringW(L"sound", nullptr, L"", legacyKeys,
        static_cast<DWORD>(std::size(legacyKeys)), settingsPath.c_str()) != 0;
    if (!MoveSettingSection(L"sound", L"clock", L"effectVolume")) return false;
    for (const wchar_t* key : { L"alarmVolume", L"alarmFile", L"useCustomAlarm" })
        if (!MoveSettingSection(L"sound", L"skea", key)) return false;
    if (hadLegacySound && !GetPrivateProfileStringW(L"sound", nullptr, L"", legacyKeys,
        static_cast<DWORD>(std::size(legacyKeys)), settingsPath.c_str()) &&
        !WritePrivateProfileStringW(L"sound", nullptr, nullptr, settingsPath.c_str())) return false;

    for (const wchar_t* key : { L"ticks", L"numbers" }) {
        if (HasIniSetting(settingsPath, L"coffe", key) &&
            !WritePrivateProfileStringW(L"coffe", key, nullptr, settingsPath.c_str())) return false;
    }
    for (const wchar_t* key : { L"markings", L"size" }) {
        if (HasIniSetting(settingsPath, L"clock", key) &&
            !WritePrivateProfileStringW(L"clock", key, nullptr, settingsPath.c_str())) return false;
    }
    return !HasIniSetting(settingsPath, L"updates", L"ignoredTag") ||
        WritePrivateProfileStringW(L"updates", L"ignoredTag", nullptr, settingsPath.c_str()) != FALSE;
}

bool ApplySettingsMigrations(const ExecutableVersion& previous, const ExecutableVersion& current) {
    constexpr ExecutableVersion layoutVersion = { 3, 2, 3 };
    if (previous < layoutVersion && layoutVersion <= current && !MigrateSettingsLayout()) return false;
    return true;
}

bool ResetAppData(const std::wstring& directory) {
    const auto target = std::filesystem::path(directory).lexically_normal();
    if (!target.is_absolute() || target.filename() != L"SkeaClock") return false;
    const DWORD attributes = GetFileAttributesW(target.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;

    // A missing version is a reset trigger only if the INI is absent or readable.
    const HANDLE ini = CreateFileW(settingsPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
        FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (ini == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) return false;
    } else {
        CloseHandle(ini);
    }

    std::error_code error;
    std::filesystem::remove_all(target, error);
    return !error;
}

std::wstring FormatExecutableVersion(const ExecutableVersion& version) {
    return std::to_wstring(version[0]) + L"." + std::to_wstring(version[1]) + L"." + std::to_wstring(version[2]);
}

bool ConfirmSettingsDowngrade(const ExecutableVersion& previous, const std::wstring& current) {
    wchar_t language[32] = {};
    GetPrivateProfileStringW(L"clock", L"language", L"ko", language,
        static_cast<DWORD>(std::size(language)), settingsPath.c_str());
    selectedLanguage = language;
    const auto versions = FormatExecutableVersion(previous) + L" \u2192 " + current;
    return ShowAppNotice(nullptr, AppError::SettingsDowngrade, versions.c_str());
}

bool PrepareSettingsVersion(const std::wstring& dataDirectory, std::wstring& versionToSave) {
    ExecutableVersion current = {}, previous = {};
    if (!ReadExecutableVersion(current)) {
        ShowAppError(nullptr, AppError::ReadExecutableVersion);
        return false;
    }

    wchar_t saved[32] = {};
    const DWORD length = GetPrivateProfileStringW(L"updates", SettingsVersionKey, MissingSettingValue, saved,
        static_cast<DWORD>(std::size(saved)), settingsPath.c_str());
    const bool hasVersion = length < std::size(saved) - 1 && ParseVersion(saved, previous);
    versionToSave = FormatExecutableVersion(current);
    if (hasVersion && previous == current) {
        if (versionToSave == saved) versionToSave.clear();
        return true;
    }
    if (hasVersion && previous > current) return ConfirmSettingsDowngrade(previous, versionToSave);

    if (std::wcscmp(saved, MissingSettingValue) == 0) {
        if (!ResetAppData(dataDirectory)) {
            ShowAppError(nullptr, AppError::ResetAppData);
            return false;
        }
        return true;
    }
    if (hasVersion && !ApplySettingsMigrations(previous, current)) {
        ShowAppError(nullptr, AppError::MigrateSettings);
        return false;
    }
    return true;
}
