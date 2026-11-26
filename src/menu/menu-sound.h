#pragma once

#include <commdlg.h>
#include "menu-file-filter.h"

#pragma comment(lib, "comdlg32.lib")

constexpr int EffectVolumeFirst = 300;
constexpr int AlarmVolumeFirst = 320;
constexpr int AlarmFileChoose = 340;
constexpr int AlarmSoundDefault = 341;
constexpr int AlarmSoundCustom = 342;
constexpr int MusicFileChoose = 343, MusicAutoPlay = 344, MusicVolumeFirst = 360;

std::wstring ChooseSoundMediaFile(HWND owner, const wchar_t* title) {
    wchar_t file[32768] = {};
    OPENFILENAMEW dialog = { sizeof(dialog) };
    dialog.hwndOwner = owner;
    const auto filter = MenuFileFilter(MediaFileTypes);
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = file;
    dialog.nMaxFile = static_cast<DWORD>(std::size(file));
    dialog.lpstrTitle = title;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog) || !SoundMediaFileSupported(file)) return {};
    return file;
}

HMENU CreateVolumeMenu(int firstCommand, int selectedVolume) {
    HMENU menu = CreatePopupMenu();
    for (int volume = 0; volume <= 100; volume += 10) {
        const std::wstring label = std::to_wstring(volume) + L"%";
        AppendMenuW(menu, MF_STRING | (volume == selectedVolume ? MF_CHECKED : 0),
            firstCommand + volume / 10, label.c_str());
    }
    return menu;
}

void AppendSoundMenus(HMENU menu) {
    HMENU effectVolumeMenu = CreateVolumeMenu(EffectVolumeFirst, effectSoundVolume);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(effectVolumeMenu), UiText().effectVolume);

    if (IsCoffeCharacter()) {
        HMENU musicMenu = CreatePopupMenu();
        AppendMenuW(musicMenu, MF_STRING, MusicFileChoose, (std::wstring(UiText().musicFileChoose) + L"...").c_str());
        if (Music::CanPlay())
            AppendMenuW(musicMenu, MF_STRING | MF_GRAYED, 0, FileName(Music::selectedFile).c_str());
        AppendMenuW(musicMenu, MF_SEPARATOR, 0, nullptr);
        HMENU musicVolumeMenu = CreateVolumeMenu(MusicVolumeFirst, Music::volume);
        AppendMenuW(musicMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(musicVolumeMenu), UiText().volume);
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(musicMenu), UiText().musicSound);
        AppendMenuW(menu, MF_STRING | (Music::autoPlay ? MF_CHECKED : 0),
            MusicAutoPlay, UiText().musicAutoPlay);
        return;
    }

    HMENU alarmSoundMenu = CreatePopupMenu();
    const auto& selection = ActiveAlarmSound();
    AppendMenuW(alarmSoundMenu, MF_STRING, AlarmFileChoose, (std::wstring(UiText().alarmFileChoose) + L"...").c_str());
    if (!selection.path.empty()) {
        const std::wstring name = FileName(selection.path);
        AppendMenuW(alarmSoundMenu, MF_STRING | (selection.useCustom ? MF_CHECKED : 0),
            AlarmSoundCustom, name.c_str());
    }
    AppendMenuW(alarmSoundMenu, MF_STRING | (!selection.useCustom ? MF_CHECKED : 0),
        AlarmSoundDefault, UiText().alarmDefault);
    AppendMenuW(alarmSoundMenu, MF_SEPARATOR, 0, nullptr);
    HMENU alarmVolumeMenu = CreateVolumeMenu(AlarmVolumeFirst, selection.volume);
    AppendMenuW(alarmSoundMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(alarmVolumeMenu), UiText().volume);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(alarmSoundMenu), UiText().alarmSound);
}
