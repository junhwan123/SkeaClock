#pragma once

#include "../app/error.h"
#include "../app/update.h"
#include "../clock/alarm.h"
#include "../interaction/interaction.h"
#include "../window/clock-startup.h"
#include "menu-exceptions.h"
#include "menu-sound.h"

void SaveSoundSettings() {
    WritePrivateProfileStringW(L"clock", L"effectVolume",
        std::to_wstring(effectSoundVolume).c_str(), settingsPath.c_str());
    for (const auto* selection : { &clockAlarmSound, &kommyAlarmSound }) {
        WritePrivateProfileStringW(selection->section, L"alarmVolume",
            std::to_wstring(selection->volume).c_str(), settingsPath.c_str());
        WritePrivateProfileStringW(selection->section, L"alarmFile", selection->path.c_str(), settingsPath.c_str());
        WritePrivateProfileStringW(selection->section, L"useCustomAlarm", selection->preferredCustom ? L"1" : L"0", settingsPath.c_str());
    }
}

void SaveMusicSettings() {
    WritePrivateProfileStringW(L"coffe", L"musicFile", Music::selectedFile.c_str(), settingsPath.c_str());
    WritePrivateProfileStringW(L"coffe", L"musicVolume", std::to_wstring(Music::volume).c_str(), settingsPath.c_str());
    WritePrivateProfileStringW(L"coffe", L"musicAutoPlay", Music::autoPlay ? L"1" : L"0", settingsPath.c_str());
}

void SaveLanguageSettings() {
    WritePrivateProfileStringW(L"clock", L"language", selectedLanguage.c_str(), settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"languageFile", customLanguagePath.c_str(), settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"useCustomLanguage", useCustomLanguage ? L"1" : L"0", settingsPath.c_str());
}

bool ReloadCustomLanguage(HWND owner) {
    if (customLanguagePath.empty()) return false;
    auto language = ReadLanguageFile(customLanguagePath);
    if (!language) {
        const std::wstring failedPath = customLanguagePath;
        customLanguagePath.clear();
        customLanguage.reset();
        if (useCustomLanguage) selectedLanguage = KoreanStrings.languageCode;
        useCustomLanguage = false;
        SaveLanguageSettings();
        ShowAppError(owner, AppError::ReloadLanguageFile, failedPath);
        return false;
    }
    customLanguage = std::move(language);
    if (useCustomLanguage) selectedLanguage = customLanguage->strings.languageCode;
    return true;
}

void SaveWindowBehaviorSettings() {
    WritePrivateProfileStringW(L"clock", L"taskbar", showTaskbar ? L"1" : L"0", settingsPath.c_str());
    const int previousCount = GetPrivateProfileIntW(L"topmostExceptions", L"count", 0, settingsPath.c_str());
    WritePrivateProfileStringW(L"topmostExceptions", L"count",
        std::to_wstring(topmostExceptions.size()).c_str(), settingsPath.c_str());
    for (size_t i = 0; i < topmostExceptions.size(); ++i) {
        const std::wstring key = L"path" + std::to_wstring(i);
        WritePrivateProfileStringW(L"topmostExceptions", key.c_str(),
            topmostExceptions[i].c_str(), settingsPath.c_str());
    }
    for (int i = static_cast<int>(topmostExceptions.size()); i < previousCount; ++i) {
        const std::wstring key = L"path" + std::to_wstring(i);
        WritePrivateProfileStringW(L"topmostExceptions", key.c_str(), nullptr, settingsPath.c_str());
    }
}

void SaveAlarms() {
    const int previousCount = GetPrivateProfileIntW(L"alarms", L"count", 0, settingsPath.c_str());
    WritePrivateProfileStringW(L"alarms", L"count", std::to_wstring(alarms.size()).c_str(), settingsPath.c_str());
    for (size_t i = 0; i < alarms.size(); ++i) {
        const std::wstring key = L"alarm" + std::to_wstring(i);
        const std::wstring value = FormatAlarmTime(alarms[i]) + L"," + std::to_wstring(alarms[i].days);
        WritePrivateProfileStringW(L"alarms", key.c_str(), value.c_str(), settingsPath.c_str());
    }
    for (int i = static_cast<int>(alarms.size()); i < previousCount; ++i) {
        const std::wstring key = L"alarm" + std::to_wstring(i);
        WritePrivateProfileStringW(L"alarms", key.c_str(), nullptr, settingsPath.c_str());
    }
}

void SaveSettings() {
    WritePrivateProfileStringW(L"clock", L"character", std::to_wstring(selectedCharacter).c_str(), settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"topmost", topmost ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"skea", L"background", showDial ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"skea", L"ticks", showTicks ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"skea", L"numbers", showNumbers ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"mute", L"hologram", Mute::hologramEnabled ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"mute", L"24hour", Mute::use24Hour ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"mute", L"background", Mute::showBackground ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"kommy", L"ticks", Kommy::showTicks ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"kommy", L"numbers", Kommy::showNumbers ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"kommy", L"time", Kommy::showTime ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"coffe", L"background", Coffe::showBackground ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"coffe", L"table", Coffe::showTable ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"coffe", L"player", Coffe::showPlayer ? L"1" : L"0", settingsPath.c_str());
    SaveMusicSettings();
    WritePrivateProfileStringW(L"clock", L"display", selectedDisplay.c_str(), settingsPath.c_str());
    SaveLanguageSettings();
    SaveSoundSettings();
    SaveWindowBehaviorSettings();
    WritePrivateProfileStringW(L"clock", L"runAtStartup", runAtStartup ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"updates", L"enabled", autoCheckUpdates ? L"1" : L"0", settingsPath.c_str());
}

#include "alarm-editor.h"

constexpr int Background = 101, Ticks = 103, Numbers = 104;
constexpr int AlwaysOnTop = 105, Exit = 106, EditLayout = 107;
constexpr int DisplayFirst = 200, MaxDisplays = 32;

constexpr int CharacterFirst = 120;

constexpr int Taskbar = 109;
constexpr int CoffeTableCommand = 115, CoffePlayerCommand = 116;

constexpr int AlarmAddCommand = 108;
constexpr int AlarmEditFirst = 1000;
constexpr int AlarmDeleteFirst = 2000;

constexpr int LanguageFileChoose = 4998, LanguageCustom = 4999;
constexpr int LanguageFirst = 5000;
static_assert(std::size(LanguagePacks) <= 0x10000 - LanguageFirst,
    "Too many languages for menu command IDs.");

bool ChooseLanguageFile(HWND owner) {
    wchar_t file[32768] = {};
    const auto filter = MenuFileFilter(LanguageFileTypes);
    OPENFILENAMEW dialog = { sizeof(OPENFILENAMEW) };
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = file;
    dialog.nMaxFile = static_cast<DWORD>(std::size(file));
    dialog.lpstrTitle = UiText().languageFileTitle;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) return false;
    auto language = ReadLanguageFile(file);
    if (!language) {
        ShowAppError(owner, AppError::ChooseLanguageFile, file);
        return false;
    }
    customLanguagePath = file;
    customLanguage = std::move(language);
    selectedLanguage = customLanguage->strings.languageCode;
    useCustomLanguage = true;
    return true;
}

HMENU CreateLanguageMenu() {
    HMENU menu = CreatePopupMenu();
    const auto& current = UiText();
    for (size_t i = 0; i < LanguagePacks.size(); ++i) {
        const auto& language = LanguagePacks[i];
        if (!language) continue;
        AppendMenuW(menu, MF_STRING | (&current == &language->strings ? MF_CHECKED : 0),
            LanguageFirst + i, language->strings.nativeName);
    }
    if (customLanguage)
        AppendMenuW(menu, MF_STRING | (useCustomLanguage ? MF_CHECKED : 0), LanguageCustom, customLanguage->strings.nativeName);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, LanguageFileChoose, (std::wstring(UiText().languageFileChoose) + L"...").c_str());
    return menu;
}

const wchar_t* CharacterMenuName(int character) {
    constexpr const wchar_t* names[] = { L"Skea clock", L"Kommy timer", L"Mute digital clock", L"Coffe player" };
    return names[character];
}

HMENU CreateCharacterMenu() {
    HMENU menu = CreatePopupMenu();
    for (int character : { SkeaCharacter, MuteCharacter, KommyCharacter, CoffeCharacter })
        AppendMenuW(menu, MF_STRING | (character == selectedCharacter ? MF_CHECKED : 0),
            CharacterFirst + character, CharacterMenuName(character));
    return menu;
}

std::wstring DisplayLabel(const DisplayInfo& display) {
    std::wstring device = display.info.szDevice;
    const size_t number = device.rfind(L"DISPLAY");
    if (number != std::wstring::npos) device = device.substr(number + 7);
    const RECT& rect = display.info.rcMonitor;
    const auto& text = UiText();
    std::wstring label = std::wstring(text.displayPrefix) + L" " + device + L"  (" +
        std::to_wstring(rect.right - rect.left) + L" × " + std::to_wstring(rect.bottom - rect.top) + L")";
    if (display.info.dwFlags & MONITORINFOF_PRIMARY) label += L" · " + std::wstring(text.displayPrimary);
    return label;
}

void ShowMenu(HWND window, POINT point) {
    DismissAlarm(window);
    if (exceptionMenu) return;
    ReloadCustomLanguage(window);
    MenuIcons icons = { GetDpiForWindow(window), {} };
    HMENU menu = CreatePopupMenu();
    MENUINFO menuInfo = { sizeof(menuInfo) };
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_CHECKORBMP;
    SetMenuInfo(menu, &menuInfo);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(CreateCharacterMenu()), CharacterMenuName(selectedCharacter));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (FirstDisplayOption() ? MF_CHECKED : 0),
        Background, IsKommyCharacter() ? UiText().timeDisplay : UiText().background);
    if (IsCoffeCharacter()) {
        AppendMenuW(menu, MF_STRING | (Coffe::showTable ? MF_CHECKED : 0), CoffeTableCommand, UiText().tableDisplay);
        AppendMenuW(menu, MF_STRING | (Coffe::showPlayer ? MF_CHECKED : 0), CoffePlayerCommand, UiText().playerDisplay);
    } else {
        AppendMenuW(menu, MF_STRING | (TickDisplayOption() ? MF_CHECKED : 0),
            Ticks, IsMuteCharacter() ? UiText().hologram : UiText().ticks);
        AppendMenuW(menu, MF_STRING | (NumberDisplayOption() ? MF_CHECKED : 0),
            Numbers, IsMuteCharacter() ? UiText().format24Hour : UiText().numbers);
        if (!IsKommyCharacter()) {
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, AlarmAddCommand, UiText().alarmAdd);
            icons.SetLast(menu, SKEA_MENU_BELL);
            for (size_t i = 0; i < alarms.size(); ++i) {
                HMENU alarmMenu = CreatePopupMenu();
                AppendMenuW(alarmMenu, MF_STRING, AlarmEditFirst + static_cast<UINT>(i), UiText().alarmEdit);
                AppendMenuW(alarmMenu, MF_STRING, AlarmDeleteFirst + static_cast<UINT>(i), UiText().alarmDelete);
                AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(alarmMenu), AlarmMenuLabel(alarms[i]).c_str());
            }
        }
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendSoundMenus(menu);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (topmost ? MF_CHECKED : 0), AlwaysOnTop, UiText().alwaysOnTop);
    AppendTopmostExceptionMenu(menu);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (runAtStartup ? MF_CHECKED : 0), RunAtStartup, UiText().runAtStartup);
    AppendMenuW(menu, MF_STRING | (showTaskbar ? MF_CHECKED : 0), Taskbar, UiText().taskbar);
    AppendMenuW(menu, MF_STRING | (layoutEditing ? MF_CHECKED : 0), EditLayout, UiText().layout);
    icons.SetLast(menu, SKEA_MENU_MOVE);
    HMENU displayMenu = CreatePopupMenu();
    const auto displays = GetDisplays();
    for (size_t i = 0; i < displays.size() && i < MaxDisplays; ++i) {
        const UINT flags = MF_STRING | (_wcsicmp(displays[i].info.szDevice, activeDisplay.c_str()) == 0 ? MF_CHECKED : 0);
        AppendMenuW(displayMenu, flags, DisplayFirst + static_cast<UINT>(i), DisplayLabel(displays[i]).c_str());
    }
    AppendMenuW(menu, MF_POPUP | (displays.empty() ? MF_GRAYED : 0),
        reinterpret_cast<UINT_PTR>(displayMenu), UiText().displaySelect);
    icons.SetLast(menu, SKEA_MENU_VM);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(CreateLanguageMenu()), L"Language");
    icons.SetLast(menu, SKEA_MENU_GLOBE);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (autoCheckUpdates ? MF_CHECKED : 0), AutoUpdateCheck, UiText().autoUpdateCheck);
    AppendMenuW(menu, MF_STRING, GithubVisit, UiText().githubVisit);
    icons.SetLast(menu, SKEA_MENU_GITHUB);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, Exit, UiText().exit);
    icons.SetLast(menu, SKEA_MENU_CLOSE);
    SetForegroundWindow(window);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        point.x, point.y, 0, window, nullptr);
    DestroyMenu(menu);
    CloseExceptionMenu();
    if (command) SendMessageW(window, WM_COMMAND, command, 0);
    PostMessageW(window, WM_NULL, 0, 0);
}

void HandleMenuCommand(HWND window, int command) {
    if (command >= CharacterFirst &&
        command < CharacterFirst + static_cast<int>(std::size(ClockCharacters))) {
        const int next = command - CharacterFirst;
        if (next == selectedCharacter) return;
        SaveCurrentLayout(window);
        headPressPending = false;
        KillTimer(window, HeadHoldTimer);
        KillTimer(window, PatHeartTimer);
        for (auto& heart : patHearts) heart.active = false;
        if (GetCapture() == window) ReleaseCapture();
        StopAlarmSound();
        StopSound();
        KillTimer(window, AnimationTimer);
        const int previous = selectedCharacter;
        const bool wasKommy = IsKommyCharacter();
        const bool wasCoffe = IsCoffeCharacter();
        ReleaseArtwork();
        selectedCharacter = next;
        const auto result = LoadArtwork(artworkDirectory);
        if (result != ClockLoadResult::Loaded) {
            ReleaseArtwork();
            selectedCharacter = previous;
            const auto restored = LoadArtwork(artworkDirectory);
            if (restored != ClockLoadResult::Loaded) {
                DestroyWindow(window);
                ShowAppError(nullptr, restored == ClockLoadResult::RuntimeError
                    ? AppError::RestoreCharacterRuntime : AppError::RestoreCharacterArtwork);
                return;
            }
            ShowAppError(window, result == ClockLoadResult::RuntimeError
                ? AppError::SwitchCharacterRuntime : AppError::SwitchCharacterArtwork);
        }
        SetSoundDirectory(artworkDirectory);
        PlayIdleSound();
        smashStarActive = false;
        if (layoutEditing || alarmEditorWindow) BeginBlankPlayback();
        PositionClock(window);
        SaveSettings();
        RenderClock(window);
        AnimateBackground(window);
        if (wasKommy != IsKommyCharacter() || wasCoffe != IsCoffeCharacter()) ValidateSavedAlarmSound(window);
        return;
    }
    if (command == TopmostExceptionAdd) {
        if (ChooseTopmostException(window)) {
            SaveSettings();
            ApplyTopmostState(window, true);
        }
        return;
    }
    if (command >= TopmostExceptionFirst &&
        command < TopmostExceptionFirst + static_cast<int>(topmostExceptionMenuPaths.size())) {
        const size_t index = command - TopmostExceptionFirst;
        if (ToggleTopmostException(topmostExceptionMenuPaths[index])) {
            SaveSettings();
            ApplyTopmostState(window, true);
        }
        return;
    }
    if (command >= EffectVolumeFirst && command <= EffectVolumeFirst + 10) {
        effectSoundVolume = (command - EffectVolumeFirst) * 10;
        SaveSettings();
        return;
    }
    if (command >= AlarmVolumeFirst && command <= AlarmVolumeFirst + 10) {
        ActiveAlarmSound().volume = (command - AlarmVolumeFirst) * 10;
        SaveSettings();
        return;
    }
    if (IsCoffeCharacter() && command >= MusicVolumeFirst && command <= MusicVolumeFirst + 10) {
        Music::SetVolume((command - MusicVolumeFirst) * 10);
        SaveSettings();
        return;
    }
    if (IsCoffeCharacter() && command == MusicFileChoose) {
        const auto file = ChooseSoundMediaFile(window, (std::wstring(UiText().musicFileChoose) + L"...").c_str());
        if (!file.empty()) Music::RequestSong(window, file);
        return;
    }
    if (IsCoffeCharacter() && command == MusicAutoPlay) {
        Music::autoPlay = !Music::autoPlay;
        SaveMusicSettings();
        return;
    }
    if (command == AlarmFileChoose) {
        const auto file = ChooseSoundMediaFile(window, UiText().alarmFileTitle);
        if (!file.empty()) RequestAlarmSound(window, ActiveAlarmSound(), file);
        return;
    }
    if (command == AlarmSoundDefault) {
        auto& selection = ActiveAlarmSound();
        CancelMediaValidation(selection.validation);
        selection.preferredCustom = selection.useCustom = false;
        SaveSoundSettings();
        return;
    }
    if (command == AlarmSoundCustom) {
        auto& selection = ActiveAlarmSound();
        if (!selection.path.empty() && !selection.useCustom) RequestAlarmSound(window, selection, selection.path);
        return;
    }
    if (command == AlarmAddCommand) {
        layoutEditing = false;
        StartBlankPlayback(window);
        OpenAlarmEditor(window);
        if (!alarmEditorWindow) StopBlankPlayback(window);
        return;
    }
    if (command >= AlarmEditFirst &&
        command < AlarmEditFirst + static_cast<int>(alarms.size())) {
        const size_t index = command - AlarmEditFirst;
        const Alarm alarm = alarms[index];
        alarms.erase(alarms.begin() + index);
        SaveAlarms();
        layoutEditing = false;
        StartBlankPlayback(window);
        OpenAlarmEditor(window, &alarm);
        if (!alarmEditorWindow) StopBlankPlayback(window);
        return;
    }
    if (command >= AlarmDeleteFirst &&
        command < AlarmDeleteFirst + static_cast<int>(alarms.size())) {
        const size_t index = command - AlarmDeleteFirst;
        alarms.erase(alarms.begin() + index);
        SaveAlarms();
        return;
    }
    if (command == EditLayout) {
        layoutEditing = true;
        StartBlankPlayback(window);
        return;
    }
    if (command >= DisplayFirst && command < DisplayFirst + MaxDisplays) {
        const auto displays = GetDisplays();
        const int index = command - DisplayFirst;
        if (index < static_cast<int>(displays.size())) {
            SaveCurrentLayout(window);
            selectedDisplay = displays[index].info.szDevice;
            PositionClock(window);
            SaveSettings();
            RenderClock(window);
        }
        return;
    }
    if (command >= LanguageFileChoose &&
        command < LanguageFirst + static_cast<int>(std::size(LanguagePacks))) {
        if (command == LanguageFileChoose) {
            if (!ChooseLanguageFile(window)) return;
        } else if (command == LanguageCustom) {
            if (!ReloadCustomLanguage(window)) return;
            useCustomLanguage = true;
            selectedLanguage = customLanguage->strings.languageCode;
        } else {
            const auto& language = LanguagePacks[command - LanguageFirst];
            if (!language) return;
            useCustomLanguage = false;
            selectedLanguage = language->strings.languageCode;
        }
        SaveSettings();
        lastStep = -1;
        if (alarmEditorWindow) {
            SetWindowTextW(alarmEditorWindow, UiText().alarmAdd);
            InvalidateRect(alarmEditorWindow, nullptr, TRUE);
        }
        PlaySoundFile(IsKommyCharacter() || IsCoffeCharacter() ? L"answer.wav" : L"smile.wav");
        return;
    }
    switch (command) {
    case Background:
        if (IsKommyCharacter()) {
            RECT bounds;
            if (!GetClockWindowRect(window, bounds)) return;
            Kommy::showTime = !Kommy::showTime;
            SetClockWindowRect(window, bounds);
        } else FirstDisplayOption() = !FirstDisplayOption();
        break;
    case Ticks:
        if (IsCoffeCharacter()) return;
        TickDisplayOption() = !TickDisplayOption();
        break;
    case Numbers:
        if (IsCoffeCharacter()) return;
        NumberDisplayOption() = !NumberDisplayOption();
        break;
    case CoffeTableCommand:
        if (!IsCoffeCharacter()) return;
        Coffe::showTable = !Coffe::showTable;
        if (!Coffe::showTable) Coffe::CancelDrag(window, true);
        break;
    case CoffePlayerCommand:
        if (!IsCoffeCharacter()) return;
        Coffe::showPlayer = !Coffe::showPlayer;
        break;
    case AlwaysOnTop: topmost = !topmost; ApplyTopmostState(window, true); break;
    case Taskbar: showTaskbar = !showTaskbar; ApplyTaskbarSetting(window); break;
    case RunAtStartup: {
        const bool enabled = !runAtStartup;
        if (SetStartupEnabled(enabled) != ERROR_SUCCESS) {
            ShowAppError(window, enabled ? AppError::RegisterStartup : AppError::RemoveStartup);
            return;
        }
        runAtStartup = enabled;
        break;
    }
    case AutoUpdateCheck: autoCheckUpdates = !autoCheckUpdates; break;
    case GithubVisit:
        ShellExecuteW(window, L"open", GithubProjectUrl, nullptr, nullptr, SW_SHOWNORMAL);
        return;
    case Exit: DestroyWindow(window); return;
    default: return;
    }
    SaveSettings();
    RenderClock(window);
    return;
}
