#pragma once


#ifndef KOMMY_LAYOUT_FILE
#define KOMMY_LAYOUT_FILE L"kommy-layout.ini"
#endif

namespace Kommy {
std::unique_ptr<AnimationImage> idle, timerAlert;
enum Clip { Idle, PatStart, PatIdle, PatEnd, Smash, TouchStart, TouchIdle, TouchChange, TouchEnd,
    BlankStart, BlankIdle, BlankEnd, Count };
std::unique_ptr<AnimationImage> actions[Count - 1][3];
constexpr float ExportScale = 1.1027f;
constexpr UINT SmashIdleEntry = 12;
constexpr UINT ReturnBlendFrames = 9;
constexpr UINT TongueDelayFrames = 12;
constexpr UINT BlankEntryFrame = 9;
Clip clip = Idle;
AnimationTiming playback;
UINT tongueRevealFrame = 0;
ULONGLONG handFadeStart = 0, touchReleaseStart = 0;
ULONGLONG timerAlertStartFrame = ULLONG_MAX;
float handFromScale = 1, handTargetScale = 1;
std::vector<PointF> idleCenters;
PointF cheekSource;
float cheekRadius = 0, touchWarpRadii[3] = {};
std::vector<PointF> touchWarpCenters[3];
float displayWidth = 512;
PointF headSource;
float headRadiusX = 0, headRadiusY = 0;

float Scale() { return displayWidth * .9f * ExportScale / idle->GetWidth(); }

bool Layered(Clip value) { return value == TouchStart || value == TouchIdle || value == TouchChange; }

AnimationImage* CurrentImage() { return clip == Idle ? idle.get() : actions[clip - 1][0].get(); }

PointF PositionForCenter(PointF center) {
    const float scale = Scale();
    return PointF(clockCenter.X - center.X * scale, clockCenter.Y - center.Y * scale);
}

PointF FixedPosition() { return PositionForCenter(idleCenters.front()); }

PointF CheekTouchCenter() {
    const float scale = Scale();
    const PointF position = FixedPosition();
    return PointF(position.X + cheekSource.X * scale, position.Y + cheekSource.Y * scale);
}

float CheekRadius() { return cheekRadius * Scale(); }

HitEllipse HeadTouchArea() {
    const float scale = Scale();
    const PointF origin = FixedPosition();
    return { PointF(origin.X + headSource.X * scale, origin.Y + headSource.Y * scale),
        headRadiusX * scale, headRadiusY * scale };
}

float HandVisibility(ULONGLONG now = GetTickCount64()) {
    float progress = std::min(1.0f, (now - handFadeStart) / static_cast<float>(PatHoldMs));
    progress = progress * progress * (3 - 2 * progress);
    return handFromScale + (handTargetScale - handFromScale) * progress;
}

void SetHandVisible(bool visible, ULONGLONG now = GetTickCount64()) {
    if (visible && tongueRevealFrame != 0) return;
    const float target = visible ? 1.0f : 0.0f;
    if (target == handTargetScale) return;
    handFromScale = HandVisibility(now);
    handTargetScale = target;
    handFadeStart = now;
}

UINT CenterReturnFrame(Clip value) {
    switch (value) {
    case Smash: return 125;
    case PatEnd: return 109;
    case TouchEnd: return 112;
    default: return UINT_MAX;
    }
}

UINT IdleEntryFrame(Clip previous) {
    if (previous == Smash) return SmashIdleEntry;
    return previous == BlankEnd ? BlankEntryFrame : 0;
}

PointF CharacterPosition() {
    if (clip == Idle) return PositionForCenter(idleCenters[idle->frame]);
    const auto& image = *CurrentImage();
    const UINT frameNumber = image.frame + 1;
    const UINT centerFrame = clip == BlankEnd ? image.GetFrameCount() : CenterReturnFrame(clip);
    if (centerFrame > image.GetFrameCount() || frameNumber + ReturnBlendFrames <= centerFrame) return FixedPosition();

    // Count back from the resume frame to preserve the idle phase at the cut.
    const UINT reference = std::max(frameNumber, centerFrame);
    const UINT remaining = image.GetFrameCount() - reference + 1;
    const UINT idleCount = idle->GetFrameCount();
    const UINT idleFrame = (IdleEntryFrame(clip) + idleCount - remaining % idleCount) % idleCount;
    const PointF from = idleCenters.front(), to = idleCenters[idleFrame];
    const float blend = 1.0f - static_cast<float>(reference - frameNumber) / ReturnBlendFrames;
    return PositionForCenter(PointF(from.X + (to.X - from.X) * blend, from.Y + (to.Y - from.Y) * blend));
}

AnimationTransition<Clip> NextAnimation() {
    switch (clip) {
    case Idle: case PatIdle: case TouchIdle: case BlankIdle: return { clip };
    case PatStart: return { PatIdle, CurrentImage()->GetFrameCount() };
    case TouchStart: return { TouchIdle, CurrentImage()->GetFrameCount() };
    case TouchChange: return { TouchEnd };
    case BlankStart: return { BlankIdle, BlankEntryFrame };
    default: return { Idle, IdleEntryFrame(clip) };
    }
}

AnimationImage* ClipLayer(Clip value, int layer) {
    if (value == Idle) return layer == 0 ? idle.get() : nullptr;
    return actions[value - 1][layer].get();
}

bool Select(UINT frame, bool immediate = false, bool initial = false) {
    AnimationPrefetchPass pass;
    const auto next = NextAnimation();
    bool ready = clip == Idle ? idle->SelectFrame(frame, immediate) == Ok
        : SelectLayerFrames(actions[clip - 1], frame, immediate);
    if (ready) playback.frame = frame;
    for (int layer = 0; layer < 3; ++layer) {
        auto* current = ClipLayer(clip, layer);
        auto* following = ClipLayer(next.clip, layer);
        if (current) current->PrepareNext(following, next.frame);
        if (following && next.clip != clip) following->Prepare(next.frame);
    }
    const auto prepare = [&](Clip value) {
        if (!PrepareLayerFrames(actions[value - 1], 0, initial) && initial) ready = false;
    };
    for (Clip value : { Smash, PatStart, TouchStart, BlankStart }) prepare(value);
    if (patActive) prepare(PatEnd);
    if (cheekDragging) prepare(TouchChange);
    if (blankPlaybackActive) prepare(BlankEnd);
    if (!timerAlert->Prepare(0, initial) && initial) ready = false;
    if (!timerAlert->Prepare(timerAlert->GetFrameCount() - 1, initial) && initial) ready = false;
    return ready;
}

void Play(Clip value, UINT first = 0, ULONGLONG tick = GetTickCount64() * AnimationFps) {
    timerAlertStartFrame = ULLONG_MAX;
    clip = value;
    playback.Begin(first, tick);
    const UINT centerFrame = CenterReturnFrame(value);
    tongueRevealFrame = centerFrame == UINT_MAX ? 0 : centerFrame + TongueDelayFrames;
    Select(first, true);
}

void StartTimerAlert(ULONGLONG now) {
    if (clip != Idle) return;
    timerAlertStartFrame = playback.Position(now);
    timerAlert->SelectFrame(0, true);
}

void BeginBlank() {
    ClearInteractionState();
    blankPlaybackActive = true;
    Play(BlankStart);
}

void EndBlank() {
    blankPlaybackActive = false;
    Play(BlankEnd);
}

void BeginPat() {
    ClearInteractionState();
    patActive = true;
    SetHandVisible(false);
    Play(PatStart);
}

void EndPat() {
    cheekWarpBuffers.Release();
    patPull = PointF();
    patActive = false;
    Play(PatEnd);
}

void BeginSmash() {
    ClearInteractionState();
    SetHandVisible(false);
    Play(Smash);
}

void BeginTouch() {
    ClearInteractionState();
    cheekDragging = true;
    SetHandVisible(false);
    Play(TouchStart);
}

void EndTouch() {
    cheekPull = PointF();
    cheekDragging = false;
    touchReleaseActive = true;
    touchReleaseStart = GetTickCount64();
    Play(TouchChange);
}

bool LoadAnimation(const std::wstring& directory, const AnimationLayout& config) {
    idle = std::make_unique<AnimationImage>();
    if (!idle->Load(directory, config.path, L"idle") || idle->firstFrame != 0 || idle->GetFrameCount() <= SmashIdleEntry ||
        idle->storedFrames != idle->GetFrameCount() ||
        config.Number(L"layout", L"width") != idle->GetWidth() ||
        config.Number(L"layout", L"height") != idle->GetHeight()) return false;
    auto loadClip = [&](const wchar_t* section, std::unique_ptr<AnimationImage>& image) {
        image = std::make_unique<AnimationImage>();
        return image->Load(directory, config.path, section) && image->firstFrame == 0 &&
            image->storedFrames == image->GetFrameCount() &&
            image->GetWidth() == idle->GetWidth() && image->GetHeight() == idle->GetHeight();
    };
    if (!loadClip(L"alert", timerAlert) || timerAlert->GetFrameCount() != 10) return false;
    const wchar_t* names[] = { L"pat_start", L"pat_idle", L"pat_end", L"smash",
        L"touch_start", L"touch_idle", L"touch_change", L"touch_end", L"blank_start", L"blank_idle", L"blank_end" };
    const wchar_t* suffixes[] = { L"_back", L"_head", L"_front" };
    for (int i = 0; i < Count - 1; ++i) {
        const Clip value = static_cast<Clip>(i + 1);
        for (int layer = 0; layer < (Layered(value) ? 3 : 1); ++layer) {
            const std::wstring section = std::wstring(names[i]) + (Layered(value) ? suffixes[layer] : L"");
            auto& image = actions[i][layer];
            if (!loadClip(section.c_str(), image) ||
                (layer && image->GetFrameCount() != actions[i][0]->GetFrameCount())) return false;
            if (layer == 1) {
                const int index = value - TouchStart;
                touchWarpRadii[index] = config.Number(section.c_str(), L"radius");
                if (!std::isfinite(touchWarpRadii[index]) || touchWarpRadii[index] <= 0 ||
                    !config.FrameCenters(section.c_str(), *image, touchWarpCenters[index])) return false;
            }
        }
    }
    if (actions[PatIdle - 1][0]->GetFrameCount() <= actions[PatStart - 1][0]->GetFrameCount() ||
        actions[TouchIdle - 1][0]->GetFrameCount() <= actions[TouchStart - 1][0]->GetFrameCount() ||
        actions[BlankIdle - 1][0]->GetFrameCount() <= BlankEntryFrame) return false;
    displayWidth = config.Number(L"layout", L"displayWidth");
    if (!std::isfinite(displayWidth) || displayWidth <= 0) return false;
    if (!config.FrameCenters(L"idle", *idle, idleCenters)) return false;
    cheekSource = PointF(config.Number(L"cheek", L"x") + .5f, config.Number(L"cheek", L"y") + .5f);
    cheekRadius = config.Number(L"cheek", L"radius");
    if (!std::isfinite(cheekSource.X) || !std::isfinite(cheekSource.Y) ||
        cheekSource.X < 0 || cheekSource.Y < 0 || cheekSource.X >= idle->GetWidth() || cheekSource.Y >= idle->GetHeight() ||
        !std::isfinite(cheekRadius) || cheekRadius <= 0) return false;
    const float sourceScale = SourceImageScale / ExportScale;
    headSource = PointF(config.Number(L"head", L"x", 246 * sourceScale - .5f) + .5f,
        config.Number(L"head", L"y", 122 * sourceScale - .5f) + .5f);
    headRadiusX = config.Number(L"head", L"radiusX", 170 * 1.3f * sourceScale / 2);
    headRadiusY = config.Number(L"head", L"radiusY", 84 * 1.3f * sourceScale / 2);
    if (!PointInImage(headSource, idle->GetWidth(), idle->GetHeight()) ||
        !std::isfinite(headRadiusX) || headRadiusX <= 0 || !std::isfinite(headRadiusY) || headRadiusY <= 0) return false;
    clip = Idle;
    const ULONGLONG now = GetTickCount64();
    timerAlertStartFrame = ULLONG_MAX;
    tongueRevealFrame = 0;
    playback.Begin(0, now * AnimationFps);
    handFromScale = handTargetScale = 1;
    handFadeStart = now;
    return Select(0, true, true);
}

UINT Advance(ULONGLONG now) {
    UINT frame = 0;
    ULONGLONG position = 0;
    for (;;) {
        const UINT count = CurrentImage()->GetFrameCount();
        position = playback.Position(now);
        const auto next = NextAnimation();
        if (next.clip == clip) {
            if (timerAlertStartFrame != ULLONG_MAX) {
                const ULONGLONG alertFrame = position - timerAlertStartFrame;
                if (alertFrame < timerAlert->GetFrameCount())
                    timerAlert->SelectFrame(static_cast<UINT>(alertFrame));
                else
                    timerAlertStartFrame = ULLONG_MAX;
            }
            frame = static_cast<UINT>(position % count);
            break;
        }
        if (position < count) { frame = static_cast<UINT>(position); break; }
        if (next.clip != Idle) Play(next.clip, next.frame, playback.End(count));
        else {
            touchReleaseActive = false;
            // Carry a reveal past the end of the action into the idle frames.
            const UINT idleReveal = tongueRevealFrame > count ? tongueRevealFrame - count : 1;
            Play(next.clip, next.frame, now * AnimationFps);
            tongueRevealFrame = playback.first + idleReveal;
        }
    }
    if (!Select(frame)) return 1;
    if (!headPressPending && tongueRevealFrame != 0 && position + 1 >= tongueRevealFrame) {
        tongueRevealFrame = 0;
        SetHandVisible(true, now);
    }
    return 16;
}

void DrawCharacter(Graphics& graphics) {
    const float scale = Scale();
    const PointF position = CharacterPosition();
    AnimationImage* animation = timerAlertStartFrame != ULLONG_MAX ? timerAlert.get() : CurrentImage();
    PointF crop = animation->FrameOffset();
    Bitmap* image = animation->Frame();
    if (Layered(clip)) {
        auto& head = *actions[clip - 1][1];
        auto& front = *actions[clip - 1][2];
        const PointF center = touchWarpCenters[clip - TouchStart][head.frame];
        const float radius = touchWarpRadii[clip - TouchStart];
        PointF pull = ClampInwardCheekPull(PointF(cheekPull.X / scale, cheekPull.Y / scale), radius);
        if (touchReleaseActive) {
            const UINT frame = static_cast<UINT>(std::min<ULONGLONG>(TouchBounceFrameCount,
                (GetTickCount64() - touchReleaseStart) * AnimationFps / FrameUnits));
            const float recovery = TouchReleasePullScale(frame);
            pull = PointF(touchReleasePullSource.X * recovery, touchReleasePullSource.Y * recovery);
        }
        DrawWarpedLayers(graphics, *animation, head, front, position, scale, center, pull, radius);
        return;
    }
    if (patActive && image) {
        const PointF center = HeadTouchArea().center;
        Bitmap* warped = WarpedHead(animation->GetWidth(), animation->GetHeight(), *animation, crop,
            PointF((center.X - position.X) / scale, (center.Y - position.Y) / scale),
            PointF(patPull.X / scale, patPull.Y / scale), PatWarpRadius / scale, true);
        if (warped) {
            image = warped;
            crop = PointF();
        }
    }
    DrawAnimationFrame(graphics, image, position, crop, scale);
}

void ReleaseAnimation() {
    idle.reset();
    timerAlert.reset();
    for (auto& layers : actions) for (auto& image : layers) image.reset();
    idleCenters.clear();
    for (auto& centers : touchWarpCenters) centers.clear();
}
}
