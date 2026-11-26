#pragma once

#include "alarm-sound.h"

std::wstring kommyDialSoundPath, kommyAlarmSoundPath;
EffectSoundChannel kommyDialSound, kommyAlarmChannel;
IMFMediaEngine* kommyAlarmMediaEngine = nullptr;

void SetTimerSoundDirectory(const std::wstring& sourceDirectory) {
    const auto directory = sourceDirectory + L"characters\\Kommy\\sound\\";
    kommyDialSoundPath = directory + L"dial.wav";
    kommyAlarmSoundPath = directory + L"alert.wav";
}

void PlayKommyDialTick() {
    if (effectSoundVolume == 0 ||
        (kommyDialSound.output && (kommyDialSound.header.dwFlags & WHDR_INQUEUE))) return;
    kommyDialSound.Play(kommyDialSoundPath, effectSoundVolume);
}

void StopKommyDialSound() { kommyDialSound.Stop(); }

void StopTimerAlarmSound() {
    ReleaseSoundMediaEngine(kommyAlarmMediaEngine);
    kommyAlarmChannel.Stop();
}

void ReleaseFinishedTimerAlarmSound() {
    if (kommyAlarmMediaEngine && kommyAlarmMediaEngine->IsEnded())
        ReleaseSoundMediaEngine(kommyAlarmMediaEngine);
    if (kommyAlarmChannel.output && (kommyAlarmChannel.header.dwFlags & WHDR_DONE))
        kommyAlarmChannel.Stop();
}

void StartTimerAlarmSound() {
    StopTimerAlarmSound();
    auto& selection = kommyAlarmSound;
    const bool custom = CanUseAlarmSound(selection);
    if (!selection.volume) return;
    if (IsKommyCharacter() && !CharacterEffectChannel().Stop()) return;
    if (custom && PlayAlarmMedia(kommyAlarmMediaEngine, selection.path, selection.volume, false)) return;
    kommyAlarmChannel.Play(kommyAlarmSoundPath, selection.volume);
}
