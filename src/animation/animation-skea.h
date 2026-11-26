#pragma once


namespace Skea {
enum Clip { Idle, Smash, PatStart, PatIdle, PatEnd, TouchStart, TouchIdle, TouchChange, TouchEnd,
    AlertStart, Alert, AlertSmash, AlertPat, AlertTouch, BlankStart, BlankIdle, BlankEnd, Count };
constexpr const wchar_t* Names[Count] = { L"idle", L"smash", L"pat_start", L"pat_idle", L"pat_end",
    L"touch_start", L"touch_idle", L"touch_change", L"touch_end",
    L"alert_start", L"alert", L"alert_smash", L"alert_pat", L"alert_touch",
    L"blank_start", L"blank_idle", L"blank_end" };
constexpr UINT StartFrames = 9, AlertStartFrames = 3, IdleSkip = 12, SmashHandFrame = 88 - 1;
constexpr UINT TouchEndHandFrame = 74 - 1;
constexpr UINT PatEndBlendFrame = 86 - 1;
std::unique_ptr<AnimationImage> images[Count][3];
std::vector<PointF> idleCenters, warpCenters[Count];
PointF cheekSource;
float scale = 1, warpRadius = 0;
Clip clip = Idle;
AnimationTiming playback;
constexpr UINT HandReturnMs = 750;
constexpr float AlarmHandTurn = 1.0f / 12.0f;
UINT handReturnDuration = HandReturnMs;
ULONGLONG handReturnStart = 0;
bool handReturnActive = false;
float handReturnStartTurns[2] = { .5f, .5f };
float alarmHandStartTurns[2] = {};
float handReturnStartLengthScales[2] = { 1.0f, 1.0f };
float alarmHandStartLengthScales[2] = { 1.0f, 1.0f };
PointF headSource;
float headRadiusX = 0, headRadiusY = 0, cheekRadius = 0;

bool Layered(Clip value) {
    return value == TouchStart || value == TouchIdle || value == TouchChange || value == AlertTouch;
}

PointF Position() {
    return PointF(clockCenter.X - idleCenters.front().X * scale, clockCenter.Y - idleCenters.front().Y * scale);
}

PointF Place(PointF point) {
    const PointF origin = Position();
    return PointF(origin.X + point.X * scale, origin.Y + point.Y * scale);
}

HitEllipse HeadTouchArea() {
    return { Place(headSource), headRadiusX * scale, headRadiusY * scale };
}

bool ShowingActionHands() {
    return (clip == Smash && playback.frame >= SmashHandFrame) ||
        (clip == TouchEnd && playback.frame >= TouchEndHandFrame);
}

bool HandsVisible() {
    return clip == Idle || clip == PatStart || clip == PatIdle || clip == PatEnd ||
        clip == AlertStart || clip == AlertPat || clip == BlankEnd || ShowingActionHands();
}

bool BlendingPatEndHandCenter() { return clip == PatEnd && playback.frame >= PatEndBlendFrame; }

PointF HandCenter(ULONGLONG now) {
    if (ShowingActionHands()) {
        const UINT first = clip == Smash ? SmashHandFrame : TouchEndHandFrame;
        return Place(idleCenters[std::min(playback.frame - first + 1, IdleSkip - 1)]);
    }
    if (BlendingPatEndHandCenter()) {
        const ULONGLONG begin = PatEndBlendFrame * FrameUnits;
        const ULONGLONG end = images[PatEnd][0]->GetFrameCount() * FrameUnits;
        const ULONGLONG elapsed = std::clamp(playback.Elapsed(now), begin, end);
        const float progress = static_cast<float>(elapsed - begin) / static_cast<float>(end - begin);
        const PointF from = idleCenters[PatEndBlendFrame], to = idleCenters[IdleSkip];
        return Place(PointF(from.X + (to.X - from.X) * progress, from.Y + (to.Y - from.Y) * progress));
    }
    if ((clip == Idle || clip == PatEnd || clip == BlankEnd) && playback.frame < idleCenters.size())
        return Place(idleCenters[playback.frame]);
    return clockCenter;
}

void StartHandReturn(float turn, ULONGLONG now, UINT duration = HandReturnMs) {
    handReturnActive = true;
    handReturnStart = now;
    handReturnDuration = duration;
    handReturnStartTurns[0] = handReturnStartTurns[1] = turn;
    handReturnStartLengthScales[0] = handReturnStartLengthScales[1] = 1;
}

void ClearInteraction() {
    ClearInteractionState();
    blankPlaybackActive = handReturnActive = false;
}

AnimationTransition<Clip> NextAnimation() {
    switch (clip) {
    case Idle: case PatIdle: case TouchIdle: case Alert: case BlankIdle: return { clip };
    case PatStart: case AlertPat: return { PatIdle, StartFrames };
    case TouchStart: case AlertTouch: return { TouchIdle, StartFrames };
    case TouchChange: return { TouchEnd };
    case AlertStart: return { Alert, AlertStartFrames };
    case AlertSmash: return { Smash, AlertStartFrames };
    case BlankStart: return { BlankIdle, StartFrames };
    case BlankEnd: return { Idle, StartFrames };
    default: return { Idle, IdleSkip };
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
    if (patActive || clip == PatStart || clip == AlertPat) prepare(PatEnd);
    if (cheekDragging || clip == TouchStart || clip == AlertTouch) prepare(TouchChange);
    if (blankPlaybackActive) prepare(BlankEnd);
    if (prepareAlarmAnimation) prepare(AlertStart);
    if (prepareAlarmAnimation || alarmActive) {
        for (Clip value : { AlertSmash, AlertPat, AlertTouch, Idle }) prepare(value);
    }
    return ready;
}

bool Play(Clip value, UINT first = 0, ULONGLONG start = GetTickCount64() * AnimationFps, bool initial = false) {
    clip = value;
    playback.Begin(first, start);
    return Select(first, true, initial);
}

UINT Advance(ULONGLONG now) {
    UINT frame = 0;
    for (;;) {
        const UINT count = images[clip][0]->GetFrameCount();
        const ULONGLONG position = playback.Position(now);
        const auto next = NextAnimation();
        if (position < count || next.clip == clip) {
            frame = static_cast<UINT>(position % count);
            break;
        }
        const ULONGLONG nextStart = playback.End(count);
        if (clip == TouchEnd) {
            touchReleaseActive = false;
            if (!ShowingActionHands()) StartHandReturn(.5f, nextStart / AnimationFps);
        }
        if (clip == AlertStart) alarmPanicActive = true;
        Play(next.clip, next.frame, nextStart);
    }
    const bool handsWereVisible = ShowingActionHands();
    if (!Select(frame)) return 1;
    if (ShowingActionHands() && !handsWereVisible) StartHandReturn(.5f, now);
    const UINT delay = playback.Delay(now);
    return clip == AlertStart || handReturnActive || BlendingPatEndHandCenter() ? std::min(delay, 16u) : delay;
}

void Begin(Clip normal, Clip alert) {
    Advance(GetTickCount64());
    const bool fromAlert = clip == Alert;
    ClearInteraction();
    Play(fromAlert ? alert : normal);
}

void BeginReaction() {
    Begin(Smash, AlertSmash);
}

void BeginPat() {
    Begin(PatStart, AlertPat);
    patActive = true;
}

void EndPat() {
    patActive = false;
    patPull = PointF();
    Play(PatEnd);
}

void BeginCheekPull() {
    Begin(TouchStart, AlertTouch);
    cheekDragging = true;
}

void EndCheekPull() {
    cheekPull = PointF();
    cheekDragging = false;
    touchReleaseActive = true;
    Play(TouchChange);
}

void BeginBlankPlayback() {
    ClearInteraction();
    blankPlaybackActive = true;
    Play(BlankStart);
}

void EndBlankPlayback() {
    blankPlaybackActive = false;
    const ULONGLONG now = GetTickCount64();
    Play(BlankEnd, 0, now * AnimationFps);
    StartHandReturn(AlarmHandTurn, now, 300);
}

void ReleaseAnimation() {
    handReturnActive = false;
    for (auto& layers : images) for (auto& image : layers) image.reset();
    idleCenters.clear();
    for (auto& centers : warpCenters) centers.clear();
}

bool LoadAnimation(const std::wstring& directory, const AnimationLayout& config) {
    scale = config.Number(L"layout", L"scale") / SourceImageScale;
    warpRadius = config.Number(L"warp", L"radius");
    cheekSource = PointF(config.Number(L"cheek", L"x") + .5f, config.Number(L"cheek", L"y") + .5f);
    if (!std::isfinite(scale) || scale <= 0 || !std::isfinite(warpRadius) || warpRadius <= 0) return false;
    const wchar_t* suffixes[] = { L"_back", L"_head", L"_front" };
    for (int i = 0; i < Count; ++i) {
        const Clip value = static_cast<Clip>(i);
        for (int layer = 0; layer < (Layered(value) ? 3 : 1); ++layer) {
            const std::wstring name = std::wstring(Names[i]) + (Layered(value) ? suffixes[layer] : L"");
            const wchar_t* section = name.c_str();
            auto& image = images[i][layer];
            image = std::make_unique<AnimationImage>();
            if (!image->Load(directory, config.path, section) || image->GetWidth() != images[Idle][0]->GetWidth() ||
                image->GetHeight() != images[Idle][0]->GetHeight() || image->firstFrame != 0 ||
                image->storedFrames != image->GetFrameCount() ||
                image->frameEnds != images[i][0]->frameEnds) return false;
            const UINT count = image->GetFrameCount();
            if (((value == Idle || value == PatEnd) && count <= PatEndBlendFrame) ||
                (value == Smash && count != 98) ||
                ((value == AlertStart || value == AlertSmash) && count != AlertStartFrames) ||
                ((value == PatStart || value == TouchStart || value == AlertPat || value == AlertTouch ||
                  value == BlankStart || value == BlankEnd) && count != StartFrames)) return false;
            if (value != Idle && layer != 1) continue;
            auto& centers = value == Idle ? idleCenters : warpCenters[i];
            if (!config.FrameCenters(section, *image, centers)) return false;
        }
    }
    const PointF anchor = idleCenters.front();
    headSource = PointF(config.Number(L"head", L"x", anchor.X - .5f) + .5f,
        config.Number(L"head", L"y", anchor.Y - .5f) + .5f);
    headRadiusX = config.Number(L"head", L"radiusX", DialRadius * .3f * 1.5f / scale);
    headRadiusY = config.Number(L"head", L"radiusY", DialRadius * .3f / scale);
    cheekRadius = config.Number(L"cheek", L"radius", DialRadius * .3f * .6f / scale);
    const auto& idle = *images[Idle][0];
    return PointInImage(cheekSource, idle.GetWidth(), idle.GetHeight()) &&
        PointInImage(headSource, idle.GetWidth(), idle.GetHeight()) &&
        std::isfinite(headRadiusX) && headRadiusX > 0 && std::isfinite(headRadiusY) && headRadiusY > 0 &&
        std::isfinite(cheekRadius) && cheekRadius > 0;
}

PointF WarpCenter() {
    if (!Layered(clip)) return warpCenters[TouchStart].front();
    return warpCenters[clip][images[clip][1]->frame];
}

void DrawCharacter(Graphics& graphics) {
    const PointF position = Position();
    auto& animation = *images[clip][0];
    if (Layered(clip)) {
        auto& head = *images[clip][1];
        auto& front = *images[clip][2];
        PointF pull = ClampInwardCheekPull(PointF(cheekPull.X / scale, cheekPull.Y / scale), warpRadius);
        if (clip == TouchChange) {
            const float recovery = TouchReleasePullScale(playback.frame);
            pull = PointF(touchReleasePullSource.X * recovery, touchReleasePullSource.Y * recovery);
        }
        DrawWarpedLayers(graphics, animation, head, front, position, scale, WarpCenter(), pull, warpRadius);
        return;
    }

    Bitmap* image = animation.Frame();
    PointF crop = animation.FrameOffset();
    if (patActive && image) {
        const UINT padding = static_cast<UINT>(std::ceil(PatWarpRadius * PatFollowFraction / (.8f / SourceImageScale))) + 2;
        const auto inset = static_cast<float>(padding);
        const PointF center(idleCenters.front().X + inset, idleCenters.front().Y + inset);
        Bitmap* warped = WarpedHead(animation.GetWidth() + padding * 2, animation.GetHeight() + padding * 2,
            animation, PointF(crop.X + inset, crop.Y + inset), center,
            PointF(patPull.X / scale, patPull.Y / scale), PatWarpRadius / scale, true);
        if (warped) {
            image = warped;
            crop = PointF(-inset, -inset);
        }
    }
    DrawAnimationFrame(graphics, image, position, crop, scale);
}
}
