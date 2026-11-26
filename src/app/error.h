#pragma once

#include <commctrl.h>
#include <string_view>
#include "../language/language.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

enum class AppError {
    UpdateAvailable = 1,
    SettingsDowngrade = 2,

    ChooseLanguageFile = 101,
    ReloadLanguageFile = 102,
    ValidateAlarmFile = 103,
    MissingAutoPlayFile = 105,
    CheckMusicSelection = 106,
    ToggleMusicPlayback = 107,
    RepeatMusicPlayback = 108,
    CheckMusicPlayback = 109,

    RegisterStartup = 201,
    RemoveStartup = 202,
    StartAlarmValidation = 203,
    AlarmValidationUnavailable = 204,
    AlarmMediaUnavailable = 205,
    SwitchCharacterArtwork = 206,
    SwitchCharacterRuntime = 207,
    StartMusicValidation = 208,
    MusicValidationUnavailable = 209,
    MusicMediaUnavailable = 210,

    AppDataPath = 301,
    UnpackAssets = 302,
    ReadExecutableVersion = 303,
    ResetAppData = 304,
    MigrateSettings = 305,
    WriteSettingsVersion = 306,
    StartupArtwork = 307,
    StartupRuntime = 308,
    CreateClockWindow = 309,
    RestoreCharacterArtwork = 310,
    RestoreCharacterRuntime = 311,
};

struct ErrorDialog {
    AppError code;
    const wchar_t* UiStrings::* message;
    UINT icon;
    const wchar_t* UiStrings::* help = nullptr;
};

constexpr ErrorDialog ErrorDialogs[] = {
    { AppError::ChooseLanguageFile, &UiStrings::languageFileError, MB_ICONWARNING, &UiStrings::languageFileHelp },
    { AppError::ReloadLanguageFile, &UiStrings::languageFileError, MB_ICONWARNING, &UiStrings::languageFileHelp },
    { AppError::ValidateAlarmFile, &UiStrings::alarmFileError, MB_ICONWARNING },
    { AppError::MissingAutoPlayFile, &UiStrings::musicFileError, MB_ICONWARNING },
    { AppError::CheckMusicSelection, &UiStrings::musicFileError, MB_ICONWARNING },
    { AppError::ToggleMusicPlayback, &UiStrings::musicFileError, MB_ICONWARNING },
    { AppError::RepeatMusicPlayback, &UiStrings::musicFileError, MB_ICONWARNING },
    { AppError::CheckMusicPlayback, &UiStrings::musicFileError, MB_ICONWARNING },

    { AppError::RegisterStartup, &UiStrings::startupError, MB_ICONWARNING, &UiStrings::startupHelp },
    { AppError::RemoveStartup, &UiStrings::startupError, MB_ICONWARNING, &UiStrings::startupHelp },
    { AppError::StartAlarmValidation, &UiStrings::mediaValidationStartError, MB_ICONWARNING },
    { AppError::AlarmValidationUnavailable, &UiStrings::mediaValidationIncomplete, MB_ICONWARNING },
    { AppError::AlarmMediaUnavailable, &UiStrings::mediaPackMissing, MB_ICONWARNING },
    { AppError::SwitchCharacterArtwork, &UiStrings::artworkLoadError, MB_ICONERROR, &UiStrings::artworkRecoveryHelp },
    { AppError::SwitchCharacterRuntime, &UiStrings::runtimeError, MB_ICONERROR },
    { AppError::StartMusicValidation, &UiStrings::mediaValidationStartError, MB_ICONWARNING },
    { AppError::MusicValidationUnavailable, &UiStrings::mediaValidationIncomplete, MB_ICONWARNING },
    { AppError::MusicMediaUnavailable, &UiStrings::mediaPackMissing, MB_ICONWARNING },

    { AppError::AppDataPath, &UiStrings::standaloneDataError, MB_ICONERROR, &UiStrings::standaloneDataHelp },
    { AppError::UnpackAssets, &UiStrings::standaloneDataError, MB_ICONERROR, &UiStrings::standaloneDataHelp },
    { AppError::ReadExecutableVersion, &UiStrings::settingsVersionError, MB_ICONERROR },
    { AppError::ResetAppData, &UiStrings::settingsVersionError, MB_ICONERROR },
    { AppError::MigrateSettings, &UiStrings::settingsVersionError, MB_ICONERROR },
    { AppError::WriteSettingsVersion, &UiStrings::settingsVersionError, MB_ICONERROR },
    { AppError::StartupArtwork, &UiStrings::artworkLoadError, MB_ICONERROR, &UiStrings::artworkRecoveryHelp },
    { AppError::StartupRuntime, &UiStrings::runtimeError, MB_ICONERROR },
    { AppError::CreateClockWindow, &UiStrings::runtimeError, MB_ICONERROR },
    { AppError::RestoreCharacterArtwork, &UiStrings::artworkLoadError, MB_ICONERROR, &UiStrings::artworkRecoveryHelp },
    { AppError::RestoreCharacterRuntime, &UiStrings::runtimeError, MB_ICONERROR },
};

const wchar_t* DialogExecutableName() {
    static const std::wstring name = [] {
        wchar_t buffer[32768];
        const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
        if (!length || length >= std::size(buffer)) return std::wstring(L"SkeaClock.exe");
        std::wstring_view executable(buffer, length);
        const auto separator = executable.find_last_of(L"\\/");
        if (separator != std::wstring_view::npos) executable.remove_prefix(separator + 1);
        return std::wstring(executable);
    }();
    return name.c_str();
}

std::wstring DialogSentence(const wchar_t* words, wchar_t ending = L'.') {
    const std::wstring_view phrase(words);
    const auto last = phrase.find_last_not_of(L" \t\r\n.!?。！？…");
    return std::wstring(phrase.substr(0, last == std::wstring_view::npos ? 0 : last + 1)) + ending;
}

void ShowAppError(HWND owner, AppError code, std::wstring_view filePath = {}) {
    const int number = static_cast<int>(code);
    for (const auto& dialog : ErrorDialogs) {
        if (dialog.code != code) continue;
        const auto& text = UiText();
        std::wstring message = DialogSentence(text.*dialog.message);
        if (dialog.help) message += L" " + DialogSentence(text.*dialog.help);
        if (number / 100 == 1 && !filePath.empty()) {
            message += L"\n\n";
            message += filePath;
        }
        message += L"\n\n(Error code: " + std::to_wstring(number) + L")";
        MessageBoxW(owner, message.c_str(), DialogExecutableName(), MB_OK | dialog.icon);
        return;
    }
}

bool ShowAppNotice(HWND owner, AppError code, const wchar_t* detail = nullptr) {
    const bool downgrade = code == AppError::SettingsDowngrade;
    if (!downgrade && code != AppError::UpdateAvailable) return false;
    const auto& text = UiText();
    const auto message = downgrade ? DialogSentence(text.settingsDowngradeWarning) :
        DialogSentence(text.updateAvailable) + L" " + DialogSentence(text.updateDownloadQuestion, L'?');
    const TASKDIALOG_BUTTON buttons[] = {
        { IDYES, downgrade ? text.settingsDowngradeContinue : text.updateDownload },
        { IDCANCEL, downgrade ? text.exit : text.updateCancel },
    };
    TASKDIALOGCONFIG dialog = {};
    dialog.cbSize = sizeof(dialog);
    dialog.hwndParent = owner;
    dialog.dwFlags = owner ? TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW : TDF_ALLOW_DIALOG_CANCELLATION;
    dialog.pszWindowTitle = DialogExecutableName();
    dialog.pszMainIcon = downgrade ? TD_WARNING_ICON : TD_INFORMATION_ICON;
    dialog.pszMainInstruction = downgrade ? message.c_str() : nullptr;
    dialog.pszContent = downgrade ? detail : message.c_str();
    dialog.cButtons = static_cast<UINT>(std::size(buttons));
    dialog.pButtons = buttons;
    dialog.nDefaultButton = IDCANCEL;
    int button = IDCANCEL;
    return SUCCEEDED(TaskDialogIndirect(&dialog, &button, nullptr, nullptr)) && button == IDYES;
}
