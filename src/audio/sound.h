#pragma once

#include "../clock/character.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <mferror.h>
#include <mmsystem.h>
#include <shlwapi.h>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <new>
#include <random>
#include <vector>

#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")

int effectSoundVolume = 80;
std::wstring commonSoundDirectory, characterSoundDirectory;
bool soundMediaFoundationStarted = false;
bool soundComInitialized = false;
HMODULE soundMediaLibrary = nullptr;
decltype(&MFStartup) soundMFStartup = nullptr;
decltype(&MFShutdown) soundMFShutdown = nullptr;
decltype(&MFCreateAttributes) soundMFCreateAttributes = nullptr;

struct SoundEndNotification {
    HWND window = nullptr;
    UINT message = 0;
    WPARAM token = 0;
};

class SoundMediaNotify final : public IMFMediaEngineNotify {
    volatile LONG references = 1;
    const SoundEndNotification ended;
public:
    explicit SoundMediaNotify(SoundEndNotification notification) : ended(notification) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** result) override {
        if (!result) return E_POINTER;
        if (id == __uuidof(IUnknown) || id == __uuidof(IMFMediaEngineNotify)) {
            *result = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }
        *result = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references); }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = InterlockedDecrement(&references);
        if (!remaining) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE EventNotify(DWORD event, DWORD_PTR, DWORD) override {
        if (ended.window && event == MF_MEDIA_ENGINE_EVENT_ENDED)
            PostMessageW(ended.window, ended.message, ended.token, 0);
        return S_OK;
    }
};

int NormalizeSoundVolume(int value) {
    value = std::clamp(value, 0, 100);
    return ((value + 5) / 10) * 10;
}

bool SoundFileExists(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool SoundMediaFileSupported(const std::wstring& path) {
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    const wchar_t* extension = path.c_str() + dot;
    const wchar_t* supported[] = {
        L".wav", L".mp3", L".m4a", L".aac", L".flac", L".wma",
        L".mp4", L".m4v", L".mov", L".wmv"
    };
    for (const wchar_t* item : supported)
        if (_wcsicmp(extension, item) == 0) return true;
    return false;
}

void SetSoundDirectory(const std::wstring& sourceDirectory) {
    commonSoundDirectory = sourceDirectory + L"common\\sound\\";
    characterSoundDirectory = sourceDirectory +
        (IsCoffeCharacter() ? L"characters\\Coffe\\sound\\" :
            IsKommyCharacter() ? L"characters\\Kommy\\sound\\" :
            IsMuteCharacter() ? L"characters\\Mute\\sound\\" : L"characters\\Skea\\sound\\");
}

std::uint16_t WaveValue16(const BYTE* data) {
    std::uint16_t value;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

std::uint32_t WaveValue32(const BYTE* data) {
    std::uint32_t value;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

bool FindPcm16Data(const std::vector<BYTE>& wave, size_t& dataOffset, size_t& dataSize,
    WAVEFORMATEX* format = nullptr) {
    if (wave.size() < 12 || std::memcmp(wave.data(), "RIFF", 4) != 0 ||
        std::memcmp(wave.data() + 8, "WAVE", 4) != 0) return false;
    bool pcm16 = false;
    dataOffset = dataSize = 0;
    for (size_t offset = 12; offset + 8 <= wave.size();) {
        const std::uint32_t chunkSize = WaveValue32(wave.data() + offset + 4);
        const size_t content = offset + 8;
        if (chunkSize > wave.size() - content) return false;
        if (std::memcmp(wave.data() + offset, "fmt ", 4) == 0 && chunkSize >= 16) {
            pcm16 = WaveValue16(wave.data() + content) == 1 &&
                WaveValue16(wave.data() + content + 14) == 16;
            if (format) std::memcpy(format, wave.data() + content, 16);
        } else if (std::memcmp(wave.data() + offset, "data", 4) == 0) {
            dataOffset = content;
            dataSize = chunkSize;
        }
        offset = content + chunkSize + (chunkSize & 1u);
    }
    return pcm16 && dataSize > 0 && dataSize % sizeof(std::int16_t) == 0;
}

bool LoadPcmWave(const std::wstring& path, int volume, std::vector<BYTE>& wave) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size = {};
    const bool validSize = GetFileSizeEx(file, &size) && size.QuadPart >= 12 &&
        size.QuadPart <= static_cast<LONGLONG>(MAXDWORD);
    if (!validSize) {
        CloseHandle(file);
        return false;
    }
    wave.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    const bool loaded = ReadFile(file, wave.data(), static_cast<DWORD>(wave.size()), &read, nullptr) &&
        static_cast<size_t>(read) == wave.size();
    CloseHandle(file);
    size_t dataOffset = 0, dataSize = 0;
    if (!loaded || !FindPcm16Data(wave, dataOffset, dataSize)) {
        wave.clear();
        return false;
    }
    volume = std::clamp(volume, 0, 100);
    for (size_t offset = dataOffset; offset < dataOffset + dataSize; offset += sizeof(std::int16_t)) {
        std::int16_t sample;
        std::memcpy(&sample, wave.data() + offset, sizeof(sample));
        sample = static_cast<std::int16_t>(sample * volume / 100);
        std::memcpy(wave.data() + offset, &sample, sizeof(sample));
    }
    return true;
}

enum class EffectSoundCategory { Start, End, Character };

struct EffectSoundChannel {
    HWAVEOUT output = nullptr;
    WAVEHDR header = {}, loopHeader = {};
    std::vector<BYTE> wave, loopWave;
    ULONGLONG generation = 0;

    bool Stop() {
        ++generation;
        if (output) {
            if (waveOutReset(output) != MMSYSERR_NOERROR) return false;
            for (auto* buffer : { &header, &loopHeader }) {
                if ((buffer->dwFlags & WHDR_PREPARED) &&
                    waveOutUnprepareHeader(output, buffer, sizeof(*buffer)) != MMSYSERR_NOERROR) return false;
            }
            if (waveOutClose(output) != MMSYSERR_NOERROR) return false;
            output = nullptr;
        }
        header = {};
        loopHeader = {};
        wave.clear();
        loopWave.clear();
        return true;
    }

    void Play(const std::wstring& path, int volume, const std::wstring& loopPath = {}) {
        if (!Stop() || volume == 0) return;
        if (!LoadPcmWave(path, volume, wave)) return;
        size_t offset = 0, size = 0;
        WAVEFORMATEX format = {};
        if (!FindPcm16Data(wave, offset, size, &format)) return;
        header.lpData = reinterpret_cast<LPSTR>(wave.data() + offset);
        header.dwBufferLength = static_cast<DWORD>(size);
        const bool loop = !loopPath.empty();
        if (loop) {
            size_t loopOffset = 0, loopSize = 0;
            WAVEFORMATEX loopFormat = {};
            if (!LoadPcmWave(loopPath, volume, loopWave) ||
                !FindPcm16Data(loopWave, loopOffset, loopSize, &loopFormat) ||
                loopFormat.nChannels != format.nChannels || loopFormat.nSamplesPerSec != format.nSamplesPerSec ||
                loopFormat.nBlockAlign != format.nBlockAlign) return;
            loopHeader.lpData = reinterpret_cast<LPSTR>(loopWave.data() + loopOffset);
            loopHeader.dwBufferLength = static_cast<DWORD>(loopSize);
        }
        if (waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
        if (waveOutPrepareHeader(output, &header, sizeof(header)) != MMSYSERR_NOERROR ||
            (loop && waveOutPrepareHeader(output, &loopHeader, sizeof(loopHeader)) != MMSYSERR_NOERROR)) {
            Stop();
            return;
        }
        if (loop) {
            loopHeader.dwFlags |= WHDR_BEGINLOOP | WHDR_ENDLOOP;
            loopHeader.dwLoops = MAXDWORD;
        }
        if (waveOutWrite(output, &header, sizeof(header)) != MMSYSERR_NOERROR ||
            (loop && waveOutWrite(output, &loopHeader, sizeof(loopHeader)) != MMSYSERR_NOERROR)) Stop();
    }
};

EffectSoundChannel effectSoundChannels[3];

EffectSoundChannel& CharacterEffectChannel() {
    return effectSoundChannels[static_cast<size_t>(EffectSoundCategory::Character)];
}

void StopKommyDialSound();
void StopTimerAlarmSound();

void StopSound() {
    for (auto& channel : effectSoundChannels) channel.Stop();
}

void StopAlarmSound();

void PlaySoundFile(const wchar_t* fileName, EffectSoundCategory category = EffectSoundCategory::Character) {
    if (IsKommyCharacter()) StopTimerAlarmSound();
    if (category != EffectSoundCategory::Character && !CharacterEffectChannel().Stop()) return;
    const auto& directory = category == EffectSoundCategory::Character ? characterSoundDirectory : commonSoundDirectory;
    effectSoundChannels[static_cast<size_t>(category)].Play(directory + fileName, effectSoundVolume);
}

void PlayIdleSound() {
    if (IsMuteCharacter() || IsKommyCharacter()) {
        CharacterEffectChannel().Play(characterSoundDirectory + L"idle.wav", effectSoundVolume);
        return;
    }
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> choice(0, 1);
    PlaySoundFile(choice(generator) ? L"idle1.wav" : L"idle2.wav");
}

void PlayInteractionEndSound(const wchar_t* interaction) {
    static std::mt19937 generator(std::random_device{}());
    static std::discrete_distribution<int> choice({ 4, 3, 3 });
    const std::wstring end = std::wstring(interaction) + L"_end.wav";
    const std::wstring voice = std::wstring(interaction) + std::to_wstring(choice(generator) + 1) + L".wav";
    PlaySoundFile(end.c_str(), EffectSoundCategory::End);
    PlaySoundFile(voice.c_str(), EffectSoundCategory::Character);
}

bool InitializeSoundMedia() {
    if (soundMediaFoundationStarted) return true;
    if (!soundMediaLibrary) {
        soundMediaLibrary = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!soundMediaLibrary) return false;
        soundMFStartup = reinterpret_cast<decltype(soundMFStartup)>(GetProcAddress(soundMediaLibrary, "MFStartup"));
        soundMFShutdown = reinterpret_cast<decltype(soundMFShutdown)>(GetProcAddress(soundMediaLibrary, "MFShutdown"));
        soundMFCreateAttributes = reinterpret_cast<decltype(soundMFCreateAttributes)>(GetProcAddress(soundMediaLibrary, "MFCreateAttributes"));
    }
    if (!soundMFStartup || !soundMFShutdown || !soundMFCreateAttributes) return false;
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) return false;
    soundComInitialized = SUCCEEDED(com);
    if (FAILED(soundMFStartup(MF_VERSION, MFSTARTUP_FULL))) {
        if (soundComInitialized) CoUninitialize();
        soundComInitialized = false;
        return false;
    }
    soundMediaFoundationStarted = true;
    return true;
}

void ReleaseSoundMediaEngine(IMFMediaEngine*& engine) {
    if (!engine) return;
    engine->Pause();
    engine->Shutdown();
    engine->Release();
    engine = nullptr;
}

HRESULT CreateAudioEngine(IMFMediaEngine** engine, decltype(&MFCreateAttributes) createAttributes,
    SoundEndNotification ended = {}) {
    SoundMediaNotify* notify = new (std::nothrow) SoundMediaNotify(ended);
    IMFAttributes* attributes = nullptr;
    IMFMediaEngineClassFactory* factory = nullptr;
    HRESULT result = notify ? createAttributes(&attributes, 1) : E_OUTOFMEMORY;
    if (SUCCEEDED(result)) result = attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, notify);
    if (SUCCEEDED(result)) result = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateInstance(
        MF_MEDIA_ENGINE_AUDIOONLY, attributes, engine);
    if (factory) factory->Release();
    if (attributes) attributes->Release();
    if (notify) notify->Release();
    return result;
}

HRESULT CreateSoundMediaEngine(IMFMediaEngine** engine, SoundEndNotification ended = {}) {
    if (!InitializeSoundMedia()) return MF_E_PLATFORM_NOT_INITIALIZED;
    return CreateAudioEngine(engine, soundMFCreateAttributes, ended);
}

HRESULT LoadSoundMediaSource(IMFMediaEngine* engine, const std::wstring& path, bool autoplay) {
    wchar_t url[32768] = {};
    DWORD urlLength = static_cast<DWORD>(std::size(url));
    HRESULT result = UrlCreateFromPathW(path.c_str(), url, &urlLength, 0);
    if (FAILED(result)) return result;
    BSTR source = SysAllocString(url);
    if (!source) return E_OUTOFMEMORY;
    result = engine->SetAutoPlay(autoplay ? TRUE : FALSE);
    if (SUCCEEDED(result)) result = engine->SetSource(source);
    SysFreeString(source);
    if (SUCCEEDED(result)) result = engine->Load();
    return result;
}

void ShutdownSoundSystem() {
    StopAlarmSound();
    StopTimerAlarmSound();
    StopKommyDialSound();
    StopSound();
    if (soundMediaFoundationStarted) soundMFShutdown();
    if (soundComInitialized) CoUninitialize();
    soundMediaFoundationStarted = false;
    soundComInitialized = false;
    if (soundMediaLibrary) FreeLibrary(soundMediaLibrary);
    soundMediaLibrary = nullptr;
    soundMFStartup = nullptr;
    soundMFShutdown = nullptr;
    soundMFCreateAttributes = nullptr;
}
