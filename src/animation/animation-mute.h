#pragma once


namespace Mute {
enum Clip { Idle, Smash, PatStart, PatIdle, PatChange, PatEnd, PatRecover,
    TouchStart, TouchIdle, TouchChange, TouchEnd, AlertStart, Alert, AlertSmash, AlertPat, AlertTouch,
    BlankStart, BlankIdle, BlankEnd, Count };
const wchar_t* const Names[] = { L"idle", L"smash", L"pat_start", L"pat_idle", L"pat_change",
    L"pat_end", L"pat_recover", L"touch_start", L"touch_idle", L"touch_change", L"touch_end",
    L"alert_start", L"alert", L"alert_smash", L"alert_pat", L"alert_touch",
    L"blank_start", L"blank_idle", L"blank_end" };
std::unique_ptr<AnimationImage> images[Count][3];
std::vector<PointF> touchIdleWarpCenters;
Clip clip = Idle;
bool smashGlitchPending = false, digitalGlitchRequested = false;
AnimationTiming playback;
ULONGLONG releaseStart = 0;
PointF backgroundSource, headSource, cheekSource, warpSource, patWarpSource;
float headRadiusX = 0, headRadiusY = 0, cheekRadius = 0, warpRadius = 0, patWarpRadius = 0, displayWidth = 512;

bool Layered(Clip value) {
    return value == TouchStart || value == TouchIdle || value == TouchChange || value == AlertTouch;
}

float Scale() { return displayWidth / images[Idle][0]->GetWidth(); }
PointF Position() {
    const float scale = Scale();
    return PointF(clockCenter.X - backgroundSource.X * scale, clockCenter.Y - backgroundSource.Y * scale);
}
PointF Place(PointF source) {
    const PointF origin = Position();
    return PointF(origin.X + source.X * Scale(), origin.Y + source.Y * Scale());
}

HitEllipse HeadTouchArea() {
    const float scale = Scale();
    return { Place(headSource), headRadiusX * scale, headRadiusY * scale };
}

void ClearInteraction() {
    ClearInteractionState();
    smashGlitchPending = digitalGlitchRequested = false;
    blankPlaybackActive = false;
}

AnimationTransition<Clip> NextAnimation() {
    switch (clip) {
    case Idle: case PatIdle: case TouchIdle: case Alert: case BlankIdle: return { clip };
    case PatStart: case AlertPat: return { PatIdle, 9 };
    case PatChange: return { PatEnd };
    case PatEnd: return { PatRecover };
    case PatRecover: case TouchEnd: return { Idle, 12 };
    case TouchStart: case AlertTouch: return { TouchIdle, 9 };
    case TouchChange: return { TouchEnd };
    case AlertStart: return { Alert, 9 };
    case AlertSmash: return { Smash, 3 };
    case BlankStart: return { BlankIdle, 9 };
    case BlankEnd: return { Idle, 9 };
    default: return { Idle };
    }
}

bool Select(UINT frame, bool immediate = false, bool initial = false) {
    AnimationPrefetchPass pass;
    const auto next = NextAnimation();
    bool ready = SelectLayerFrames(images[clip], frame, immediate);
    if (ready) playback.frame = frame;
    PrepareFollowingLayers(images[clip], images[next.clip], next.frame);
    if (next.clip != clip) PrepareLayerFrames(images[next.clip], next.frame);

    const auto prepare = [&](Clip value) {
        if (!PrepareLayerFrames(images[value], 0, initial) && initial) ready = false;
    };
    for (Clip value : { Smash, PatStart, TouchStart, BlankStart }) prepare(value);
    if (patActive) prepare(PatChange);
    if (cheekDragging || clip == TouchStart || clip == AlertTouch) prepare(TouchChange);
    if (blankPlaybackActive) prepare(BlankEnd);
    if (prepareAlarmAnimation) prepare(AlertStart);
    if (prepareAlarmAnimation || alarmActive) {
        for (Clip value : { AlertSmash, AlertPat, AlertTouch, Idle }) prepare(value);
    }
    if (ready && smashGlitchPending && clip == Smash && frame >= 4) {
        smashGlitchPending = false;
        digitalGlitchRequested = true;
    }
    return ready;
}

bool Play(Clip value, UINT first = 0, ULONGLONG start = GetTickCount64() * AnimationFps, bool initial = false) {
    clip = value;
    playback.Begin(first, start);
    return Select(first, true, initial);
}

bool Reset(bool initial = false) {
    ClearInteraction();
    return Play(Idle, 0, GetTickCount64() * AnimationFps, initial);
}

UINT Advance(ULONGLONG now) {
    UINT frame = 0;
    for (;;) {
        const UINT count = images[clip][0]->GetFrameCount();
        const ULONGLONG position = playback.Position(now);
        const auto next = NextAnimation();
        if (next.clip == clip) {
            frame = static_cast<UINT>(position % count);
            break;
        }
        if (position < count) { frame = static_cast<UINT>(position); break; }
        const ULONGLONG nextStart = playback.End(count);
        if (clip == TouchEnd) touchReleaseActive = false;
        if (clip == AlertStart) alarmPanicActive = true;
        Play(next.clip, next.frame, nextStart);
    }
    const bool ready = Select(frame);
    return ready ? playback.Delay(now) : 1;
}

void Begin(Clip normal, Clip alert) {
    Advance(GetTickCount64());
    const bool fromAlert = clip == Alert;
    ClearInteraction();
    Play(fromAlert ? alert : normal);
}

void BeginAlarm() {
    ClearInteraction();
    alarmActive = true;
    Play(AlertStart);
}
void BeginBlank() {
    ClearInteraction();
    blankPlaybackActive = true;
    Play(BlankStart);
}
void EndBlank() {
    blankPlaybackActive = false;
    Play(BlankEnd);
}
void BeginSmash() {
    Begin(Smash, AlertSmash);
    smashGlitchPending = true;
}
void BeginPat() {
    Advance(GetTickCount64());
    const bool fromAlert = clip == Alert;
    const UINT first = clip == PatChange ? images[PatStart][0]->GetFrameCount() - 1 - playback.frame : 0;
    ClearInteraction();
    patActive = true;
    Play(fromAlert ? AlertPat : PatStart, fromAlert ? 0 : first);
}
void EndPat() {
    patActive = false;
    Play(PatChange);
}
void BeginTouch() {
    Begin(TouchStart, AlertTouch);
    cheekDragging = true;
}
void EndTouch() {
    cheekPull = PointF();
    cheekDragging = false;
    touchReleaseActive = true;
    releaseStart = GetTickCount64();
    Play(TouchChange);
}

void Release() {
    for (auto& layers : images) for (auto& image : layers) image.reset();
    touchIdleWarpCenters.clear();
}

bool LoadAnimation(const std::wstring& directory, const AnimationLayout& config) {
    const float width = config.Number(L"layout", L"width"), height = config.Number(L"layout", L"height");
    auto point = [&](const wchar_t* section, PointF& value, PointF fallback = PointF(-1, -1)) {
        value = PointF(config.Number(section, L"x", fallback.X), config.Number(section, L"y", fallback.Y));
        return std::isfinite(value.X) && std::isfinite(value.Y) &&
            value.X >= 0 && value.Y >= 0 && value.X < width && value.Y < height;
    };
    displayWidth = config.Number(L"layout", L"displayWidth", 512);
    backgroundSource = PointF(config.Number(L"layout", L"x", std::nanf("")), config.Number(L"layout", L"y", std::nanf("")));
    if (width <= 0 || height <= 0 || !std::isfinite(displayWidth) || displayWidth <= 0 ||
        !std::isfinite(backgroundSource.X) || !std::isfinite(backgroundSource.Y) ||
        !point(L"head", headSource) || !point(L"cheek", cheekSource) ||
        !point(L"warp", warpSource, cheekSource) || !point(L"pat_warp", patWarpSource, headSource)) return false;
    const float headRadius = config.Number(L"head", L"radius");
    headRadiusX = config.Number(L"head", L"radiusX", headRadius * 1.5f);
    headRadiusY = config.Number(L"head", L"radiusY", headRadius);
    cheekRadius = config.Number(L"cheek", L"radius");
    warpRadius = config.Number(L"warp", L"radius");
    patWarpRadius = config.Number(L"pat_warp", L"radius", PatWarpRadius * width / displayWidth);
    if (!std::isfinite(headRadiusX) || !std::isfinite(headRadiusY) || !std::isfinite(cheekRadius) ||
        !std::isfinite(warpRadius) || !std::isfinite(patWarpRadius) || headRadiusX <= 0 ||
        headRadiusY <= 0 || cheekRadius <= 0 || warpRadius <= 0 || patWarpRadius <= 0) return false;
    touchIdleWarpCenters.clear();
    const wchar_t* suffixes[] = { L"_back", L"_head", L"_front" };
    for (int i = 0; i < Count; ++i) {
        const Clip value = static_cast<Clip>(i);
        for (int layer = 0; layer < (Layered(value) ? 3 : 1); ++layer) {
            const std::wstring section = std::wstring(Names[i]) + (Layered(value) ? suffixes[layer] : L"");
            auto& image = images[i][layer];
            image = std::make_unique<AnimationImage>();
            if (!image->Load(directory, config.path, section.c_str()) || image->GetWidth() != width ||
                image->GetHeight() != height || image->firstFrame != 0 ||
                image->storedFrames != image->GetFrameCount()) return false;
            const UINT count = image->GetFrameCount();
            if ((value == Idle && count <= 12) ||
                ((value == PatIdle || value == TouchIdle || value == Alert || value == BlankIdle) && count <= 9) ||
                ((value == PatStart || value == PatChange || value == TouchStart || value == AlertStart ||
                  value == AlertPat || value == AlertTouch ||
                  value == BlankStart || value == BlankEnd) && count != 9) ||
                (value == AlertSmash && count != 3) ||
                ((value == PatRecover || value == TouchChange) && count != 12) ||
                (layer && count != images[i][0]->GetFrameCount())) return false;
            if (value == TouchIdle && layer == 1 &&
                !config.FrameCenters(section.c_str(), *image, touchIdleWarpCenters)) return false;
        }
    }
    return Reset(true);
}

void DrawCharacter(Graphics& graphics) {
    const PointF position = Position();
    const float scale = Scale();
    auto& image = *images[clip][0];
    if (Layered(clip)) {
        auto& head = *images[clip][1];
        auto& front = *images[clip][2];
        const PointF center = clip == TouchIdle ? touchIdleWarpCenters[head.frame] : warpSource;
        PointF pull = ClampInwardCheekPull(PointF(cheekPull.X / scale, cheekPull.Y / scale), warpRadius);
        if (touchReleaseActive) {
            const UINT frame = static_cast<UINT>(std::min<ULONGLONG>(TouchBounceFrameCount,
                (GetTickCount64() - releaseStart) * AnimationFps / FrameUnits));
            const float recovery = TouchReleasePullScale(frame);
            pull = PointF(touchReleasePullSource.X * recovery, touchReleasePullSource.Y * recovery);
        }
        DrawWarpedLayers(graphics, image, head, front, position, scale, center, pull, warpRadius);
    } else if (patActive && image.Frame()) {
        Bitmap* warped = WarpedHead(image.GetWidth(), image.GetHeight(), image, image.FrameOffset(),
            patWarpSource, PointF(patPull.X / scale, patPull.Y / scale), patWarpRadius, true);
        DrawAnimationFrame(graphics, warped ? warped : image.Frame(), position,
            warped ? PointF() : image.FrameOffset(), scale);
    } else {
        DrawAnimationFrame(graphics, image.Frame(), position, image.FrameOffset(), scale);
    }
}
}
