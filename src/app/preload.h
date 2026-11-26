#pragma once

#include "../clock/alarm.h"
#include "../menu/menu.h"
#include "../window/clock-startup.h"
#include "embedded-assets.h"
#include "error.h"
#include "settings-migration.h"

int ReadIntSetting(const wchar_t* section, const wchar_t* key, int fallback) {
    if (!HasIniSetting(settingsPath, section, key))
        WritePrivateProfileStringW(section, key, std::to_wstring(fallback).c_str(), settingsPath.c_str());
    return GetPrivateProfileIntW(section, key, fallback, settingsPath.c_str());
}

std::wstring ReadTextSetting(const wchar_t* section, const wchar_t* key, const wchar_t* fallback = L"") {
    if (!HasIniSetting(settingsPath, section, key))
        WritePrivateProfileStringW(section, key, fallback, settingsPath.c_str());
    wchar_t value[32768] = {};
    GetPrivateProfileStringW(section, key, fallback, value, static_cast<DWORD>(std::size(value)), settingsPath.c_str());
    return value;
}

void LoadLanguageSettings() {
    selectedLanguage = ReadTextSetting(L"clock", L"language", KoreanStrings.languageCode);
    customLanguagePath = ReadTextSetting(L"clock", L"languageFile");
    useCustomLanguage = ReadIntSetting(L"clock", L"useCustomLanguage", 0) != 0 && !customLanguagePath.empty();
    ReloadCustomLanguage(nullptr);
}

void LoadWindowBehaviorSettings(const std::wstring& path) {
    showTaskbar = ReadIntSetting(L"clock", L"taskbar", 0) != 0;
    topmostExceptions.clear();
    const int count = std::clamp(static_cast<int>(ReadIntSetting(L"topmostExceptions", L"count", 0)),
        0, static_cast<int>(MaximumTopmostExceptions));
    for (int i = 0; i < count; ++i) {
        wchar_t value[32768] = {};
        const std::wstring key = L"path" + std::to_wstring(i);
        GetPrivateProfileStringW(L"topmostExceptions", key.c_str(), L"", value,
            static_cast<DWORD>(std::size(value)), path.c_str());
        if (*value && !IsTopmostExceptionPath(value)) topmostExceptions.emplace_back(value);
    }
}

void LoadAlarms() {
    alarms.clear();
    const int count = std::clamp(static_cast<int>(ReadIntSetting(L"alarms", L"count", 0)),
        0, static_cast<int>(MaximumAlarms));
    for (int i = 0; i < count; ++i) {
        wchar_t value[64] = {};
        const std::wstring key = L"alarm" + std::to_wstring(i);
        GetPrivateProfileStringW(L"alarms", key.c_str(), L"", value, 64, settingsPath.c_str());
        int hour = -1, minute = -1;
        unsigned days = 0;
        if (swscanf_s(value, L"%d:%d,%u", &hour, &minute, &days) == 3 &&
            hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && days > 0 && days <= 0x7f)
            alarms.push_back({ hour, minute, days });
    }
}

void LoadSoundSettings() {
    effectSoundVolume = NormalizeSoundVolume(ReadIntSetting(L"clock", L"effectVolume", 80));
    for (auto* selection : { &clockAlarmSound, &kommyAlarmSound }) {
        const int previousVolume = selection == &kommyAlarmSound ? effectSoundVolume : 80;
        selection->volume = NormalizeSoundVolume(ReadIntSetting(selection->section, L"alarmVolume", previousVolume));
        selection->path = ReadTextSetting(selection->section, L"alarmFile");
        selection->preferredCustom = ReadIntSetting(selection->section, L"useCustomAlarm", 0) != 0;
    }
}

void LoadSettings() {
    constexpr int defaultCharacter = SkeaCharacter;
    selectedCharacter = ReadIntSetting(L"clock", L"character", defaultCharacter);
    if (selectedCharacter < 0 || selectedCharacter >= static_cast<int>(std::size(ClockCharacters))) selectedCharacter = defaultCharacter;
    topmost = ReadIntSetting(L"clock", L"topmost", 1) != 0;
    showDial = ReadIntSetting(L"skea", L"background", 1) != 0;
    showTicks = ReadIntSetting(L"skea", L"ticks", 1) != 0;
    showNumbers = ReadIntSetting(L"skea", L"numbers", 1) != 0;
    Mute::hologramEnabled = ReadIntSetting(L"mute", L"hologram", 1) != 0;
    Mute::use24Hour = ReadIntSetting(L"mute", L"24hour", 1) != 0;
    Mute::showBackground = ReadIntSetting(L"mute", L"background", 0) != 0;
    Kommy::showTicks = ReadIntSetting(L"kommy", L"ticks", 1) != 0;
    Kommy::showNumbers = ReadIntSetting(L"kommy", L"numbers", 1) != 0;
    Kommy::showTime = ReadIntSetting(L"kommy", L"time", 0) != 0;
    Coffe::showBackground = ReadIntSetting(L"coffe", L"background", 1) != 0;
    Coffe::showTable = ReadIntSetting(L"coffe", L"table", 1) != 0;
    Coffe::showPlayer = ReadIntSetting(L"coffe", L"player", 1) != 0;
    Music::selectedFile = ReadTextSetting(L"coffe", L"musicFile");
    Music::volume = NormalizeSoundVolume(ReadIntSetting(L"coffe", L"musicVolume", 80));
    Music::autoPlay = ReadIntSetting(L"coffe", L"musicAutoPlay", 0) != 0;
    LoadLanguageSettings();
    LoadSoundSettings();
    LoadWindowBehaviorSettings(settingsPath);
    autoCheckUpdates = ReadIntSetting(L"updates", L"enabled", 1) != 0;
    selectedDisplay = ReadTextSetting(L"clock", L"display");
    const bool savedStartup = HasIniSetting(settingsPath, L"clock", L"runAtStartup");
    runAtStartup = ReadIntSetting(L"clock", L"runAtStartup", !savedStartup && IsStartupEnabled() ? 1 : 0) != 0;
    LoadAlarms();
}

HICON AppIcon() {
    return LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
}

bool PreloadApplication(ULONG_PTR& gdiplusToken) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    LoadBuiltInLanguages();

    std::wstring dataDirectory, sourceDirectory;
    if (!GetAppDataPaths(dataDirectory, sourceDirectory, SourceImageScale != 1.0f)) {
        ShowAppError(nullptr, AppError::AppDataPath);
        return false;
    }
    settingsPath = dataDirectory + L"\\skea-clock.ini";
    std::wstring versionToSave;
    if (!PrepareSettingsVersion(dataDirectory, versionToSave)) return false;
    if (!UnpackEmbeddedAssets(sourceDirectory)) {
        LoadLanguageSettings();
        ShowAppError(nullptr, AppError::UnpackAssets);
        return false;
    }
    LoadSettings();
    GdiplusStartupInput input;
    if (GdiplusStartup(&gdiplusToken, &input, nullptr) != Ok) return false;

    SetSoundDirectory(sourceDirectory);
    SetTimerSoundDirectory(sourceDirectory);
    CoffeFiles::playbackDirectory = sourceDirectory + L"characters\\Coffe\\sound\\";
    artworkDirectory = sourceDirectory;
    const auto result = LoadArtwork(sourceDirectory);
    if (result == ClockLoadResult::Loaded) {
        if (versionToSave.empty() || WritePrivateProfileStringW(L"updates", SettingsVersionKey,
            versionToSave.c_str(), settingsPath.c_str())) return true;
        ShowAppError(nullptr, AppError::WriteSettingsVersion);
    } else {
        const bool runtimeError = result == ClockLoadResult::RuntimeError;
        ShowAppError(nullptr, runtimeError ? AppError::StartupRuntime : AppError::StartupArtwork);
    }
    ReleaseArtwork();
    GdiplusShutdown(gdiplusToken);
    return false;
}
