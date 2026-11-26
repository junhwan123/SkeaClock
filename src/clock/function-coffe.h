#pragma once

#include "../animation/animation-coffe.h"
#include "../audio/media-validation.h"
#include "character.h"

namespace CoffeFiles {
std::wstring playbackDirectory;
std::vector<std::wstring> pendingRemoval;

bool DeleteMusicCopy(const std::wstring& file) {
    if (file.empty() || DeleteFileW(file.c_str())) return true;
    const DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

void ReleaseMusicCopy(std::wstring& file) {
    if (!DeleteMusicCopy(file)) pendingRemoval.push_back(std::move(file));
    file.clear();
}

void CleanMusicCopies() {
    pendingRemoval.erase(std::remove_if(pendingRemoval.begin(), pendingRemoval.end(), DeleteMusicCopy),
        pendingRemoval.end());
}

struct MusicCopy {
    std::wstring source, directory, file;
    BY_HANDLE_FILE_INFORMATION original = {};
    std::atomic<bool> cancelled{ false }, complete{ false };
    HRESULT result = E_FAIL;

    ~MusicCopy() { DeleteMusicCopy(file); }
};

HRESULT WriteMusicCopy(MusicCopy& job, HANDLE input, const std::wstring& extension) {
    GUID id = {};
    HRESULT result = CoCreateGuid(&id);
    if (FAILED(result)) return result;
    wchar_t name[39] = {};
    if (!StringFromGUID2(id, name, static_cast<int>(std::size(name)))) return E_FAIL;
    const std::wstring path = job.directory + L"coffe-playback-" + name + extension;
    HANDLE output = CreateFileW(path.c_str(), GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (output == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());
    // Let Windows clean up an incomplete copy if the app closes during the write.
    FILE_DISPOSITION_INFO disposition{ TRUE };
    const bool deleteOnClose = SetFileInformationByHandle(output, FileDispositionInfo, &disposition, sizeof(disposition)) != FALSE;
    job.file = path;
    BYTE buffer[64 * 1024];
    while (!job.cancelled) {
        DWORD read = 0, written = 0;
        if (!ReadFile(input, buffer, sizeof(buffer), &read, nullptr)) {
            result = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
        if (!read) break;
        if (!WriteFile(output, buffer, read, &written, nullptr)) {
            result = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
        if (written != read) {
            result = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            break;
        }
    }
    if (job.cancelled) result = E_ABORT;
    if (SUCCEEDED(result) && deleteOnClose) {
        disposition.DeleteFile = FALSE;
        if (!SetFileInformationByHandle(output, FileDispositionInfo, &disposition, sizeof(disposition)))
            result = HRESULT_FROM_WIN32(GetLastError());
    }
    if (!CloseHandle(output) && SUCCEEDED(result)) result = HRESULT_FROM_WIN32(GetLastError());
    return result;
}

HRESULT CreateMusicCopy(MusicCopy& job) {
    HANDLE input = CreateFileW(job.source.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (input == INVALID_HANDLE_VALUE) return AudioFileError(GetLastError());
    BY_HANDLE_FILE_INFORMATION before = {};
    HRESULT result = ReadAudioFileInfo(input, before);
    if (result == S_OK) {
        result = SameAudioFile(before, job.original)
            ? WriteMusicCopy(job, input, job.source.substr(job.source.find_last_of(L'.'))) : S_FALSE;
    }
    CloseHandle(input);
    return result == S_OK ? CheckAudioFile(job.source, job.original) : result;
}

void CALLBACK CopyMusic(PTP_CALLBACK_INSTANCE instance, void* context) {
    std::unique_ptr<std::shared_ptr<MusicCopy>> holder(static_cast<std::shared_ptr<MusicCopy>*>(context));
    const auto job = *holder;
    CallbackMayRunLong(instance);
    try {
        if (!job->cancelled) job->result = CreateMusicCopy(*job);
    } catch (...) { job->result = E_FAIL; }
    job->complete.store(true, std::memory_order_release);
}

std::shared_ptr<MusicCopy> BeginMusicCopy(const std::wstring& source, const BY_HANDLE_FILE_INFORMATION& original) {
    auto job = std::make_shared<MusicCopy>();
    job->source = source;
    job->directory = playbackDirectory;
    job->original = original;
    auto holder = std::make_unique<std::shared_ptr<MusicCopy>>(job);
    if (!TrySubmitThreadpoolCallback(CopyMusic, holder.get(), nullptr)) return nullptr;
    holder.release();
    return job;
}
}

#include "../audio/music.h"

namespace Coffe {
constexpr float TableOffsetY = 12.0f;
constexpr float GrooveWidth = 1.25f;
constexpr ULONGLONG RecordRotationMs = 30000;
const Color GrooveColor(156, 255, 255, 255);
const RectF RecordButtonBounds = DialCornerCircleBounds(DialRadius + DialFrameWidth, 3.0f);
const PointF RecordIconCenter(RecordButtonBounds.X + RecordButtonBounds.Width / 2,
    RecordButtonBounds.Y + RecordButtonBounds.Height / 2);
const RectF RecordUnavailableCircle(RecordIconCenter.X - 8, RecordIconCenter.Y - 8, 16, 16);
const PointF RecordUnavailableStart(RecordIconCenter.X - 5.65f, RecordIconCenter.Y - 5.65f);
const PointF RecordUnavailableEnd(RecordIconCenter.X + 5.65f, RecordIconCenter.Y + 5.65f);
const RectF RecordPauseBars[] = {
    { RecordIconCenter.X - 6, RecordIconCenter.Y - 7, 4, 14 },
    { RecordIconCenter.X + 2, RecordIconCenter.Y - 7, 4, 14 },
};
const PointF RecordPlayTriangle[] = {
    { RecordIconCenter.X - 5.0f, RecordIconCenter.Y - 8.0f },
    { RecordIconCenter.X + 8.0f, RecordIconCenter.Y },
    { RecordIconCenter.X - 5.0f, RecordIconCenter.Y + 8.0f },
};
bool showBackground = true, showTable = true, showPlayer = true;

struct TableProp {
    const wchar_t* file;
    float offsetX;
    float sizeMultiplier;
    std::unique_ptr<Bitmap> image;
    AnimationImage* animation = nullptr;
    RectF bounds;
};

std::unique_ptr<GraphicsPath> recordGrooves;
std::unique_ptr<Bitmap> table;
float tableTop = 0.0f;
TableProp coffeeBag{ L"coffee-bag.png", -45.0f, 1.25f, {} };
TableProp coffeePot{ L"coffee-pot.png", 45.0f, 1.25f, {} };
TableProp blankCup{ L"coffee-cup-blank.png", 0.0f, 2.0f, {} };
TableProp filledCup{ L"coffee-cup-filled.png", 0.0f, 2.0f, {} };
TableProp catCup{ nullptr, 0.0f, 1.0f, {} };
bool draggingProp = false;
PointF dragStart, dragPointer;

// Keep the record controls in sync with music across character changes.
bool recordSpinning = false;
ULONGLONG recordStartedAt = 0;
float pausedRecordAngle = 0;

float RecordAngle(ULONGLONG now) {
    if (!recordSpinning) return pausedRecordAngle;
    const float elapsed = static_cast<float>((now - recordStartedAt) % RecordRotationMs);
    return std::fmod(pausedRecordAngle - elapsed * (360.0f / RecordRotationMs), 360.0f);
}

void SetRecordSpinning(bool spinning) {
    if (recordSpinning == spinning) return;
    const ULONGLONG now = GetTickCount64();
    if (spinning) recordStartedAt = now;
    else pausedRecordAngle = RecordAngle(now);
    recordSpinning = spinning;
}

float TableTop() { return tableTop; }

const TableProp& SelectedFilledCup() { return useCatCup ? catCup : filledCup; }

const TableProp* VisibleTableProp(ULONGLONG now) {
    if (!IsMakingCoffee(now)) return &coffeeBag;
    switch (clip) {
    case Coffee1Start:
    case Coffee1Idle: return &coffeePot;
    case Coffee2Start:
    case Coffee2Idle: return &blankCup;
    case Coffee3Idle: return &SelectedFilledCup();
    default: return nullptr;
    }
}

void PrepareTablePropBounds(TableProp& prop) {
    const auto* animation = prop.animation;
    const float propScale = (animation ? ImageScale() : scale) * prop.sizeMultiplier;
    const float width = (animation ? animation->GetWidth() : prop.image->GetWidth()) * propScale;
    const float height = (animation ? animation->GetHeight() : prop.image->GetHeight()) * propScale;
    prop.bounds = RectF(clockCenter.X + prop.offsetX - width / 2, TableTop() + 9.0f - height, width, height);
}

RectF TablePropBounds(const TableProp& prop) {
    RectF bounds = prop.bounds;
    if (draggingProp) bounds.Offset(dragPointer.X - dragStart.X, dragPointer.Y - dragStart.Y);
    return bounds;
}

void BeginBlank() {
    ClearInteractionState();
    CoffeEffects::Reset();
    draggingProp = false;
    cupClickedAt = 0;
    blankPlaybackActive = true;
    PlayClip(BlankStart, GetTickCount64());
}

void EndBlank() {
    blankPlaybackActive = false;
    PlayClip(BlankEnd, GetTickCount64());
}

ClockLoadResult Load(const std::wstring& directory) {
    if (!LoadAnimation(directory)) return ClockLoadResult::AssetError;
    catCup.animation = catCupAnimation.get();
    recordGrooves = std::make_unique<GraphicsPath>();
    constexpr int ringCount = 10, arcsPerRing = 3;
    constexpr float innerRadius = 48.0f + GrooveWidth / 2;
    constexpr float outerRadius = 96.0f - GrooveWidth / 2;
    constexpr float sweeps[arcsPerRing] = { 68.0f, 52.0f, 78.0f };
    // Keep the pattern stable when returning to Coffe during playback.
    std::mt19937 grooveRandom;
    std::uniform_real_distribution<float> lengthScale(0.55f, 0.78f);
    for (int ring = 0; ring < ringCount; ++ring) {
        const float radius = innerRadius + (outerRadius - innerRadius) * ring / (ringCount - 1);
        const RectF bounds(clockCenter.X - radius, clockCenter.Y - radius, radius * 2, radius * 2);
        for (int arc = 0; arc < arcsPerRing; ++arc) {
            const float start = static_cast<float>((ring * 29 + arc * (360 / arcsPerRing)) % 360);
            const float sweep = sweeps[(ring + arc) % arcsPerRing] * lengthScale(grooveRandom);
            recordGrooves->StartFigure();
            recordGrooves->AddArc(bounds, start, sweep);
        }
    }
    if (recordGrooves->GetLastStatus() != Ok) return ClockLoadResult::RuntimeError;
    const auto loadProp = [&directory](const wchar_t* file) {
        auto image = std::make_unique<Bitmap>((directory + file).c_str());
        if (image->GetLastStatus() != Ok) image.reset();
        return image;
    };
    table = loadProp(L"table.png");
    if (!table) return ClockLoadResult::AssetError;
    tableTop = clockCenter.Y + DialRadius - static_cast<float>(table->GetHeight()) + TableOffsetY;
    for (auto* prop : { &coffeeBag, &coffeePot, &blankCup, &filledCup }) {
        prop->image = loadProp(prop->file);
        if (!prop->image) return ClockLoadResult::AssetError;
        PrepareTablePropBounds(*prop);
    }
    if (catCup.animation) PrepareTablePropBounds(catCup);
    return PlayClip(Idle, GetTickCount64(), 0, true) ? ClockLoadResult::Loaded : ClockLoadResult::AssetError;
}

void Release() {
    CoffeEffects::Reset();
    draggingProp = false;
    catCup.animation = nullptr;
    ReleaseAnimation();
    recordGrooves.reset();
    table.reset();
    for (auto* prop : { &coffeeBag, &coffeePot, &blankCup, &filledCup }) prop->image.reset();
}

void DrawSmoke(Graphics& graphics, const RectF& cup, const ImageAttributes* attributes = nullptr) {
    if (!smoke) return;
    auto* image = smoke->Frame();
    if (!image) return;
    // Both cups share a rim height measured from their base.
    const float cupScale = scale * filledCup.sizeMultiplier;
    const PointF rim(cup.X + cup.Width / 2 + 5.0f * cupScale, cup.GetBottom() - 18.0f * cupScale);
    const PointF offset = smoke->FrameOffset();
    const float imageScale = ImageScale();
    const RectF bounds(rim.X + (offset.X - smoke->GetWidth() / 2.0f) * imageScale,
        rim.Y + (offset.Y - smoke->GetHeight()) * imageScale, image->GetWidth() * imageScale, image->GetHeight() * imageScale);
    graphics.DrawImage(image, bounds, 0, 0, static_cast<float>(image->GetWidth()),
        static_cast<float>(image->GetHeight()), UnitPixel, attributes);
}

void DrawTableProp(Graphics& graphics, const TableProp& prop, RectF bounds,
    const ImageAttributes* attributes = nullptr) {
    auto* image = prop.animation ? prop.animation->Frame() : prop.image.get();
    if (!image) return;
    if (prop.animation) {
        const float propScale = bounds.Width / prop.animation->GetWidth();
        const PointF offset = prop.animation->FrameOffset();
        bounds = RectF(bounds.X + offset.X * propScale, bounds.Y + offset.Y * propScale,
            image->GetWidth() * propScale, image->GetHeight() * propScale);
    }
    graphics.DrawImage(image, bounds, 0, 0, static_cast<float>(image->GetWidth()),
        static_cast<float>(image->GetHeight()), UnitPixel, attributes);
}

void DrawCupFade(Graphics& graphics, ULONGLONG now) {
    if (!CupIsFading(now)) return;
    const float progress = static_cast<float>(now - cupClickedAt) / CupFadeMs;
    const auto& cup = SelectedFilledCup();
    RectF bounds = TablePropBounds(cup);
    const ColorMatrix opacity = {{
        { 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0 },
        { 0, 0, 1, 0, 0 },
        { 0, 0, 0, 1.0f - progress, 0 },
        { 0, 0, 0, 0, 1 },
    }};
    ImageAttributes attributes;
    attributes.SetColorMatrix(&opacity);
    DrawSmoke(graphics, bounds, &attributes);
    bounds.Inflate(bounds.Width * progress / 2, bounds.Height * progress / 2);
    DrawTableProp(graphics, cup, bounds, &attributes);
}

void Draw(Graphics& graphics) {
    const ULONGLONG now = GetTickCount64();
    graphics.SetInterpolationMode(InterpolationModeBilinear);
    const GraphicsState clipState = clockDial->BeginScene(graphics, showBackground);
    if (showBackground) {
        const GraphicsState grooveState = graphics.Save();
        graphics.TranslateTransform(clockCenter.X, clockCenter.Y);
        graphics.RotateTransform(RecordAngle(now));
        graphics.TranslateTransform(-clockCenter.X, -clockCenter.Y);
        Pen grooves(GrooveColor, GrooveWidth);
        graphics.DrawPath(&grooves, recordGrooves.get());
        graphics.Restore(grooveState);
    }
    DrawCharacter(graphics);
    if (showTable) {
        const float height = static_cast<float>(table->GetHeight());
        graphics.DrawImage(table.get(), clockCenter.X - DialRadius, TableTop(), DialRadius * 2, height);
        if (const auto* prop = VisibleTableProp(now)) {
            const RectF bounds = TablePropBounds(*prop);
            if (clip == Coffee3Idle) DrawSmoke(graphics, bounds);
            DrawTableProp(graphics, *prop, bounds);
        } else {
            DrawCupFade(graphics, now);
        }
    }
    clockDial->EndScene(graphics, clipState, showBackground, showPlayer);
    if (showPlayer) {
        graphics.FillEllipse(&clockDial->background, RecordButtonBounds);
        SolidBrush icon(GrooveColor);
        if (!Music::CanPlay()) {
            Pen unavailable(GrooveColor, 2.25f);
            graphics.DrawEllipse(&unavailable, RecordUnavailableCircle);
            graphics.DrawLine(&unavailable, RecordUnavailableStart, RecordUnavailableEnd);
        } else if (recordSpinning) {
            graphics.FillRectangles(&icon, RecordPauseBars, static_cast<INT>(std::size(RecordPauseBars)));
        } else {
            graphics.FillPolygon(&icon, RecordPlayTriangle, static_cast<INT>(std::size(RecordPlayTriangle)));
        }
    }
}
}
