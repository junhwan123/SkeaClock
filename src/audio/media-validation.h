#pragma once

#include "sound.h"

#include <atomic>
#include <memory>
#include <mfreadwrite.h>
#include <wrl/client.h>

enum class MediaValidationResult { Valid, FileError, MediaUnavailable, PlaybackUnavailable };

MediaValidationResult ClassifyMediaFailure(HRESULT error) {
    if (error == MF_E_PLATFORM_NOT_INITIALIZED || error == REGDB_E_CLASSNOTREG ||
        error == CLASS_E_CLASSNOTAVAILABLE || error == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) ||
        error == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND) || error == CO_E_DLLNOTFOUND)
        return MediaValidationResult::MediaUnavailable;
    if (error == MF_E_NO_AUDIO_PLAYBACK_DEVICE || error == MF_E_AUDIO_PLAYBACK_DEVICE_IN_USE ||
        error == MF_E_AUDIO_PLAYBACK_DEVICE_INVALIDATED || error == MF_E_AUDIO_SERVICE_NOT_RUNNING ||
        error == E_OUTOFMEMORY || error == E_ACCESSDENIED ||
        error == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
        return MediaValidationResult::PlaybackUnavailable;
    return MediaValidationResult::FileError;
}

HRESULT AudioFileError(DWORD error) {
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_DELETE_PENDING)
        return S_FALSE;
    return HRESULT_FROM_WIN32(error);
}

HRESULT ReadAudioFileInfo(HANDLE file, BY_HANDLE_FILE_INFORMATION& info) {
    if (!GetFileInformationByHandle(file, &info)) return AudioFileError(GetLastError());
    return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_FALSE : S_OK;
}

HRESULT ReadAudioFileInfo(const std::wstring& path, BY_HANDLE_FILE_INFORMATION& current) {
    if (path.empty()) return S_FALSE;
    HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (file == INVALID_HANDLE_VALUE) return AudioFileError(GetLastError());
    const HRESULT result = ReadAudioFileInfo(file, current);
    CloseHandle(file);
    return result;
}

bool SameAudioFile(const BY_HANDLE_FILE_INFORMATION& current, const BY_HANDLE_FILE_INFORMATION& originalFile) {
    return current.dwVolumeSerialNumber == originalFile.dwVolumeSerialNumber &&
        current.nFileIndexHigh == originalFile.nFileIndexHigh && current.nFileIndexLow == originalFile.nFileIndexLow &&
        current.nFileSizeHigh == originalFile.nFileSizeHigh && current.nFileSizeLow == originalFile.nFileSizeLow &&
        CompareFileTime(&current.ftLastWriteTime, &originalFile.ftLastWriteTime) == 0;
}

HRESULT CheckAudioFile(const std::wstring& path, const BY_HANDLE_FILE_INFORMATION& accepted) {
    BY_HANDLE_FILE_INFORMATION current = {};
    const HRESULT result = ReadAudioFileInfo(path, current);
    return result == S_OK && !SameAudioFile(current, accepted) ? S_FALSE : result;
}

struct MediaValidationJob {
    std::wstring path;
    BY_HANDLE_FILE_INFORMATION original = {};
    std::atomic<bool> cancelled{ false }, complete{ false };
    std::atomic<ULONGLONG> progress{ GetTickCount64() };
    MediaValidationResult result = MediaValidationResult::PlaybackUnavailable;
};

constexpr ULONGLONG MediaValidationIdleMs = 15000;

std::uint32_t MediaBigEndian32(const BYTE* bytes) {
    return (static_cast<std::uint32_t>(bytes[0]) << 24) |
        (static_cast<std::uint32_t>(bytes[1]) << 16) |
        (static_cast<std::uint32_t>(bytes[2]) << 8) | bytes[3];
}

bool MediaFileIsTruncated(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false; // The decoder reports open errors.
    LARGE_INTEGER size = {};
    BYTE header[64] = {};
    DWORD read = 0;
    bool truncated = false;
    if (GetFileSizeEx(file, &size) && ReadFile(file, header, sizeof(header), &read, nullptr) && read >= 12) {
        const ULONGLONG length = static_cast<ULONGLONG>(size.QuadPart);
        if (!std::memcmp(header, "RIFF", 4) && !std::memcmp(header + 8, "WAVE", 4)) {
            // WAV decoders may accept EOF even when a declared data chunk was cut short.
            const ULONGLONG end = static_cast<ULONGLONG>(WaveValue32(header + 4)) + 8;
            truncated = end > length;
            for (ULONGLONG offset = 12; !truncated && offset + 8 <= std::min(end, length);) {
                LARGE_INTEGER position; position.QuadPart = static_cast<LONGLONG>(offset);
                if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN) ||
                    !ReadFile(file, header, 8, &read, nullptr) || read != 8) break;
                const ULONGLONG bytes = WaveValue32(header + 4);
                truncated = bytes > length - offset - 8;
                offset += 8 + bytes + (bytes & 1);
            }
        } else {
            ULONGLONG offset = 0;
            if (!std::memcmp(header, "ID3", 3) && !(header[6] & 128) && !(header[7] & 128) &&
                !(header[8] & 128) && !(header[9] & 128)) {
                offset = 10ULL + (header[6] << 21) + (header[7] << 14) + (header[8] << 7) + header[9];
                if (header[3] == 4 && (header[5] & 16)) offset += 10; // ID3v2.4 footer.
                LARGE_INTEGER position; position.QuadPart = static_cast<LONGLONG>(offset);
                if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN) ||
                    !ReadFile(file, header, sizeof(header), &read, nullptr)) read = 0;
            }
            if (read >= 64 && offset < length && header[0] == 255 && (header[1] & 224) == 224 &&
                (header[1] & 6) == 2 && (header[1] & 24) != 8) {
                // Read the first MPEG Layer III frame's Xing/Info or VBRI byte count.
                const bool mpeg1 = (header[1] & 24) == 24, mono = (header[3] & 192) == 192;
                const size_t tag = 4 + (mpeg1 ? (mono ? 17 : 32) : (mono ? 9 : 17));
                std::uint32_t declared = 0;
                if (!std::memcmp(header + tag, "Xing", 4) || !std::memcmp(header + tag, "Info", 4)) {
                    const auto flags = MediaBigEndian32(header + tag + 4);
                    if (flags & 2) declared = MediaBigEndian32(header + tag + 8 + ((flags & 1) ? 4 : 0));
                } else if (!std::memcmp(header + 36, "VBRI", 4) && header[40] == 0 && header[41] == 1) {
                    declared = MediaBigEndian32(header + 46);
                }
                truncated = declared > length - offset;
            }
        }
    }
    CloseHandle(file);
    return truncated;
}

HRESULT DecodeMediaAudio(MediaValidationJob& job,
    decltype(&MFCreateSourceReaderFromURL) createReader, decltype(&MFCreateMediaType) createType) {
    using Microsoft::WRL::ComPtr;
    constexpr DWORD audioStream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    if (MediaFileIsTruncated(job.path)) return MF_E_INVALID_FILE_FORMAT;
    ComPtr<IMFSourceReader> reader;
    HRESULT result = createReader(job.path.c_str(), nullptr, &reader);
    if (FAILED(result)) return result;
    job.progress = GetTickCount64();
    ComPtr<IMFMediaType> video;
    const bool hasVideo = SUCCEEDED(reader->GetNativeMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0, &video));
    PROPVARIANT duration = {};
    ULONGLONG expected = 0;
    if (!hasVideo && SUCCEEDED(reader->GetPresentationAttribute(static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE),
        MF_PD_DURATION, &duration)) && duration.vt == VT_UI8) expected = duration.uhVal.QuadPart;
    PropVariantClear(&duration);
    result = reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    if (SUCCEEDED(result)) result = reader->SetStreamSelection(audioStream, TRUE);
    ComPtr<IMFMediaType> pcm;
    if (SUCCEEDED(result)) result = createType(&pcm);
    if (SUCCEEDED(result)) result = pcm->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(result)) result = pcm->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (SUCCEEDED(result)) result = reader->SetCurrentMediaType(audioStream, nullptr, pcm.Get());
    if (FAILED(result)) return result;
    bool decodedAudio = false;
    LONGLONG decodedEnd = 0;
    while (!job.cancelled) {
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        ComPtr<IMFSample> sample;
        result = reader->ReadSample(audioStream, 0, nullptr, &flags, &timestamp, &sample);
        if (FAILED(result)) return result;
        if (flags & MF_SOURCE_READERF_ERROR) return MF_E_INVALID_FILE_FORMAT;
        if (sample) {
            DWORD bytes = 0;
            result = sample->GetTotalLength(&bytes);
            if (FAILED(result)) return result;
            if (bytes) {
                decodedAudio = true;
                LONGLONG sampleDuration = 0;
                sample->GetSampleDuration(&sampleDuration);
                decodedEnd = std::max(decodedEnd, timestamp + std::max(0LL, sampleDuration));
            }
        }
        job.progress = GetTickCount64();
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            if (!decodedAudio) return MF_E_INVALID_FILE_FORMAT;
            // Container duration can include video-only tails; compare audio-only files.
            const ULONGLONG tolerance = std::max(5000000ULL, expected / 100);
            if (expected > tolerance && static_cast<ULONGLONG>(decodedEnd) < expected - tolerance)
                return MF_E_INVALID_FILE_FORMAT;
            return S_OK;
        }
    }
    return E_ABORT;
}

MediaValidationResult CheckSilentPlayback(MediaValidationJob& job, decltype(&MFCreateAttributes) createAttributes) {
    IMFMediaEngine* engine = nullptr;
    HRESULT result = CreateAudioEngine(&engine, createAttributes);
    if (FAILED(result)) {
        const auto failure = ClassifyMediaFailure(result);
        return failure == MediaValidationResult::FileError ? MediaValidationResult::PlaybackUnavailable : failure;
    }

    auto status = MediaValidationResult::PlaybackUnavailable;
    result = engine->SetMuted(TRUE);
    if (SUCCEEDED(result)) result = LoadSoundMediaSource(engine, job.path, true);
    const ULONGLONG start = GetTickCount64();
    job.progress = start;
    if (FAILED(result)) status = ClassifyMediaFailure(result);
    else while (!job.cancelled && GetTickCount64() - start < MediaValidationIdleMs) {
        IMFMediaError* error = nullptr;
        engine->GetError(&error);
        if (error) {
            status = ClassifyMediaFailure(error->GetExtendedErrorCode());
            error->Release();
            break;
        }
        if (engine->GetReadyState() >= MF_MEDIA_ENGINE_READY_HAVE_METADATA) {
            if (!engine->HasAudio()) {
                status = MediaValidationResult::FileError;
                break;
            }
            if (engine->GetCurrentTime() > 0 && (!engine->IsPaused() || engine->IsEnded())) {
                status = MediaValidationResult::Valid;
                break;
            }
        }
        Sleep(100);
    }
    ReleaseSoundMediaEngine(engine);
    return status;
}

void CALLBACK ValidateMedia(PTP_CALLBACK_INSTANCE instance, void* context) {
    std::unique_ptr<std::shared_ptr<MediaValidationJob>> holder(static_cast<std::shared_ptr<MediaValidationJob>*>(context));
    const auto job = *holder;
    CallbackMayRunLong(instance);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    HMODULE platform = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE readwrite = LoadLibraryExW(L"mfreadwrite.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    const auto startup = reinterpret_cast<decltype(&MFStartup)>(platform ? GetProcAddress(platform, "MFStartup") : nullptr);
    const auto shutdown = reinterpret_cast<decltype(&MFShutdown)>(platform ? GetProcAddress(platform, "MFShutdown") : nullptr);
    const auto createAttributes = reinterpret_cast<decltype(&MFCreateAttributes)>(platform ? GetProcAddress(platform, "MFCreateAttributes") : nullptr);
    const auto createType = reinterpret_cast<decltype(&MFCreateMediaType)>(platform ? GetProcAddress(platform, "MFCreateMediaType") : nullptr);
    const auto createReader = reinterpret_cast<decltype(&MFCreateSourceReaderFromURL)>(
        readwrite ? GetProcAddress(readwrite, "MFCreateSourceReaderFromURL") : nullptr);
    bool started = false;
    bool background = false;
    try {
        if (FAILED(com) || job->cancelled) job->result = MediaValidationResult::PlaybackUnavailable;
        else if (!startup || !shutdown || !createAttributes || !createType || !createReader || FAILED(startup(MF_VERSION, MFSTARTUP_FULL)))
            job->result = MediaValidationResult::MediaUnavailable;
        else {
            started = true;
            const HRESULT original = ReadAudioFileInfo(job->path, job->original);
            if (FAILED(original)) job->result = ClassifyMediaFailure(original);
            else if (original == S_FALSE || !SoundMediaFileSupported(job->path)) job->result = MediaValidationResult::FileError;
            else job->result = CheckSilentPlayback(*job, createAttributes);

            if (job->result == MediaValidationResult::Valid && !job->cancelled) {
                job->progress = GetTickCount64();
                background = SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN) != FALSE;
                const HRESULT decoded = DecodeMediaAudio(*job, createReader, createType);
                if (FAILED(decoded)) job->result = ClassifyMediaFailure(decoded);
                else {
                    const HRESULT unchanged = CheckAudioFile(job->path, job->original);
                    if (FAILED(unchanged)) job->result = ClassifyMediaFailure(unchanged);
                    else if (unchanged == S_FALSE) job->result = MediaValidationResult::FileError;
                }
            }
        }
    } catch (...) { job->result = MediaValidationResult::PlaybackUnavailable; }
    if (started) shutdown();
    if (readwrite) FreeLibrary(readwrite);
    if (platform) FreeLibrary(platform);
    if (SUCCEEDED(com)) CoUninitialize();
    if (background) SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
    job->complete.store(true, std::memory_order_release);
}

void CancelMediaValidation(std::shared_ptr<MediaValidationJob>& job) {
    if (job) job->cancelled = true;
    job.reset();
}

bool BeginMediaValidation(std::shared_ptr<MediaValidationJob>& task, std::wstring path) {
    CancelMediaValidation(task);
    auto job = std::make_shared<MediaValidationJob>();
    job->path = std::move(path);
    auto holder = std::make_unique<std::shared_ptr<MediaValidationJob>>(job);
    if (!TrySubmitThreadpoolCallback(ValidateMedia, holder.get(), nullptr)) return false;
    holder.release();
    task = std::move(job);
    return true;
}

bool PollMediaValidation(const std::shared_ptr<MediaValidationJob>& task, MediaValidationResult& result) {
    if (!task) return false;
    if (task->complete.load(std::memory_order_acquire)) {
        result = task->result;
        return true;
    }
    const ULONGLONG lastProgress = task->progress.load();
    if (GetTickCount64() - lastProgress < MediaValidationIdleMs) return false;
    task->cancelled = true;
    result = MediaValidationResult::PlaybackUnavailable;
    return true;
}
