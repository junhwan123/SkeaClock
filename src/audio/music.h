#pragma once

#include <optional>

#include "media-validation.h"
#include "../app/error.h"

void SaveMusicSettings();

namespace Music {
constexpr UINT EndedMessage = WM_APP + 6;
enum class PlaybackState { Paused, Waiting, Playing };

std::wstring selectedFile, playbackFile;
int volume = 80;
bool autoPlay = false;
bool verified = false, restoring = false;
BY_HANDLE_FILE_INFORMATION selectedOriginal = {};
std::shared_ptr<MediaValidationJob> validation;
std::shared_ptr<CoffeFiles::MusicCopy> copying;
IMFMediaEngine* player = nullptr;
PlaybackState playbackState = PlaybackState::Paused;
WPARAM playerToken = 0;

bool CanPlay() { return verified; }
bool PlaybackRequested() { return playbackState != PlaybackState::Paused; }

void SetVolume(int value) {
    volume = NormalizeSoundVolume(value);
    if (player) player->SetVolume(volume / 100.0);
}

void StopPlayback() {
    playbackState = PlaybackState::Paused;
    ReleaseSoundMediaEngine(player);
    if (copying) copying->cancelled = true;
    copying.reset();
    CoffeFiles::ReleaseMusicCopy(playbackFile);
}

void Shutdown() {
    CancelMediaValidation(validation);
    StopPlayback();
    CoffeFiles::CleanMusicCopies();
}

void ClearSelection() {
    StopPlayback();
    verified = false;
    selectedFile.clear();
    autoPlay = false;
    SaveMusicSettings();
}

void RequestSong(HWND owner, const std::wstring& path, bool existing = false) {
    if (!existing && verified && path == selectedFile) {
        CancelMediaValidation(validation);
        return;
    }
    restoring = existing;
    if (!BeginMediaValidation(validation, path)) ShowAppError(owner, AppError::StartMusicValidation);
}

void ValidateSavedSong(HWND owner) {
    if (!selectedFile.empty()) RequestSong(owner, selectedFile, true);
    else if (autoPlay) {
        ClearSelection();
        ShowAppError(owner, AppError::MissingAutoPlayFile);
    }
}

std::optional<AppError> CheckSelection(std::wstring& failedPath) {
    MediaValidationResult result;
    if (!PollMediaValidation(validation, result)) return std::nullopt;
    const auto job = std::move(validation);
    const bool startPlaying = restoring && autoPlay;
    restoring = false;
    if (result == MediaValidationResult::MediaUnavailable) return AppError::MusicMediaUnavailable;
    if (result == MediaValidationResult::PlaybackUnavailable) return AppError::MusicValidationUnavailable;
    if (result == MediaValidationResult::FileError) {
        failedPath = job->path;
        if (job->path == selectedFile) ClearSelection();
        return AppError::CheckMusicSelection;
    }

    StopPlayback();
    selectedFile = job->path;
    selectedOriginal = job->original;
    verified = true;
    SaveMusicSettings();
    copying = CoffeFiles::BeginMusicCopy(selectedFile, selectedOriginal);
    if (!copying) {
        failedPath = job->path;
        return AppError::CheckMusicSelection;
    }
    if (startPlaying) playbackState = PlaybackState::Waiting;
    return std::nullopt;
}

HRESULT LoadPlaybackCopy(HWND owner) {
    HRESULT result = CreateSoundMediaEngine(&player, { owner, EndedMessage, ++playerToken });
    if (SUCCEEDED(result)) result = player->SetLoop(FALSE);
    if (SUCCEEDED(result)) result = player->SetVolume(volume / 100.0);
    if (SUCCEEDED(result)) result = player->SetPreload(MF_MEDIA_ENGINE_PRELOAD_AUTOMATIC);
    if (SUCCEEDED(result)) result = LoadSoundMediaSource(player, playbackFile, false);
    if (FAILED(result)) StopPlayback();
    return result;
}

HRESULT CheckPlayback(HWND owner) {
    CoffeFiles::CleanMusicCopies();
    if (copying && copying->complete.load(std::memory_order_acquire)) {
        const auto job = std::move(copying);
        if (job->result == S_FALSE) {
            ClearSelection();
            return S_FALSE;
        }
        if (FAILED(job->result)) {
            StopPlayback();
            return job->result;
        }
        playbackFile = std::move(job->file);
        job->file.clear();
    }
    if (!player) {
        if (playbackState != PlaybackState::Waiting || playbackFile.empty()) return S_OK;
        const HRESULT original = CheckAudioFile(selectedFile, selectedOriginal);
        if (original == S_FALSE) ClearSelection();
        if (original != S_OK) {
            playbackState = PlaybackState::Paused;
            return original;
        }
        const HRESULT opened = LoadPlaybackCopy(owner);
        if (FAILED(opened)) return opened;
    }
    IMFMediaError* error = nullptr;
    player->GetError(&error);
    if (error) {
        const HRESULT result = error->GetExtendedErrorCode();
        error->Release();
        StopPlayback();
        return FAILED(result) ? result : MF_E_INVALID_FILE_FORMAT;
    }
    if (playbackState != PlaybackState::Waiting ||
        player->GetReadyState() < MF_MEDIA_ENGINE_READY_HAVE_METADATA) return S_OK;
    HRESULT result = player->HasAudio() ? S_OK : MF_E_INVALID_FILE_FORMAT;
    if (SUCCEEDED(result) && player->IsEnded()) result = player->SetCurrentTime(0.0);
    if (SUCCEEDED(result)) result = player->Play();
    if (FAILED(result)) StopPlayback();
    else playbackState = PlaybackState::Playing;
    return result;
}

HRESULT SetPlaying(HWND owner, bool playing) {
    if (!playing) {
        if (player && playbackState == PlaybackState::Playing && FAILED(player->Pause())) StopPlayback();
        playbackState = PlaybackState::Paused;
        return S_OK;
    }
    if (!CanPlay()) return S_FALSE;
    const HRESULT original = CheckAudioFile(selectedFile, selectedOriginal);
    if (original == S_FALSE) ClearSelection();
    if (original != S_OK) return original;
    if (playbackFile.empty() && !copying) {
        copying = CoffeFiles::BeginMusicCopy(selectedFile, selectedOriginal);
        if (!copying) return E_FAIL;
    }
    playbackState = PlaybackState::Waiting;
    return CheckPlayback(owner);
}

HRESULT RepeatPlayback(HWND owner, WPARAM token) {
    if (!player || token != playerToken || playbackState != PlaybackState::Playing ||
        player->IsSeeking() || !player->IsEnded()) return S_OK;
    const HRESULT result = SetPlaying(owner, true);
    if (FAILED(result)) StopPlayback();
    return result;
}
}
