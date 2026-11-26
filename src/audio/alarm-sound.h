#pragma once

#include "media-validation.h"
#include "../app/error.h"

struct AlarmSoundSelection {
    const wchar_t* section;
    std::wstring path;
    bool preferredCustom = false;
    bool useCustom = false;
    int volume = 80;
    BY_HANDLE_FILE_INFORMATION original = {};
    std::shared_ptr<MediaValidationJob> validation;

    explicit AlarmSoundSelection(const wchar_t* name) : section(name) {}
};
AlarmSoundSelection clockAlarmSound(L"skea"), kommyAlarmSound(L"kommy");
IMFMediaEngine* alarmMediaEngine = nullptr;

void SaveSoundSettings();

AlarmSoundSelection& ActiveAlarmSound() {
    return IsKommyCharacter() ? kommyAlarmSound : clockAlarmSound;
}

void RequestAlarmSound(HWND window, AlarmSoundSelection& selection, const std::wstring& path) {
    if (!BeginMediaValidation(selection.validation, path))
        ShowAppError(window, AppError::StartAlarmValidation);
}

void ValidateSavedAlarmSound(HWND window) {
    if (IsCoffeCharacter()) return;
    auto& selection = ActiveAlarmSound();
    if (selection.preferredCustom && !selection.useCustom && !selection.validation)
        RequestAlarmSound(window, selection, selection.path);
}

void CancelAlarmSoundValidation() {
    for (auto* selection : { &clockAlarmSound, &kommyAlarmSound })
        CancelMediaValidation(selection->validation);
}

void CheckAlarmSoundValidation(HWND window) {
    for (auto* selection : { &clockAlarmSound, &kommyAlarmSound }) {
        MediaValidationResult result;
        if (!PollMediaValidation(selection->validation, result)) continue;
        const auto job = std::move(selection->validation);
        if (result == MediaValidationResult::Valid) {
            selection->path = job->path;
            selection->original = job->original;
            selection->preferredCustom = selection->useCustom = true;
            SaveSoundSettings();
            continue;
        }
        if (result == MediaValidationResult::FileError) {
            if (selection->path == job->path) {
                selection->preferredCustom = selection->useCustom = false;
                selection->path.clear();
                SaveSoundSettings();
            }
            ShowAppError(window, AppError::ValidateAlarmFile, job->path);
        } else if (result == MediaValidationResult::MediaUnavailable) {
            ShowAppError(window, AppError::AlarmMediaUnavailable);
        } else {
            ShowAppError(window, AppError::AlarmValidationUnavailable);
        }
    }
}

bool CanUseAlarmSound(AlarmSoundSelection& selection) {
    if (!selection.useCustom) return false;
    const HRESULT original = CheckAudioFile(selection.path, selection.original);
    if (original == S_FALSE) {
        selection.preferredCustom = selection.useCustom = false;
        selection.path.clear();
        SaveSoundSettings();
    }
    return original == S_OK;
}

void StopAlarmSound() { ReleaseSoundMediaEngine(alarmMediaEngine); }

bool PlayAlarmMedia(IMFMediaEngine*& engine, const std::wstring& path, int volume, bool repeat) {
    if (!SoundFileExists(path)) return false;
    HRESULT result = CreateSoundMediaEngine(&engine);
    if (SUCCEEDED(result)) result = engine->SetLoop(repeat ? TRUE : FALSE);
    if (SUCCEEDED(result)) result = engine->SetVolume(volume / 100.0);
    if (SUCCEEDED(result)) result = LoadSoundMediaSource(engine, path, true);
    if (FAILED(result)) ReleaseSoundMediaEngine(engine);
    return SUCCEEDED(result);
}

void StartClockAlarmSound() {
    StopAlarmSound();
    auto& selection = clockAlarmSound;
    const bool custom = CanUseAlarmSound(selection);
    if (!selection.volume) return;
    const auto fallback = commonSoundDirectory + L"base_alert.wav";
    if (custom && PlayAlarmMedia(alarmMediaEngine, selection.path, selection.volume, true)) return;
    PlayAlarmMedia(alarmMediaEngine, fallback, selection.volume, true);
}
