#pragma once
#include <algorithm>
#include <array>
#include <climits>
#include <memory>
#include "clock-image-scale.h"

constexpr float DialRadius = 108.0f;
constexpr float TouchRadius = DialRadius * 0.3f;
constexpr float HeadTouchRadiusX = TouchRadius * 1.5f;
constexpr float PatWarpRadius = TouchRadius * 3;
constexpr float PatFollowFraction = 0.15f;
constexpr float CheekTouchRadius = TouchRadius * 0.6f;
constexpr float CharacterScale = 0.8f / SourceImageScale;
constexpr float HandScale = 0.9f / SourceImageScale;
constexpr float TimeHandLengthScales[2] = { .95f, 1.05f };
constexpr float MaxInwardCheekPull = 0.3f;
constexpr float CheekSeamOverlap = SourceImageScale;
constexpr UINT HandReturnMs = 750;
constexpr UINT SmashStartFrame = 4;
constexpr UINT AngryCutFrame = 14;
constexpr UINT AngryAltStartFrame = 23;
constexpr UINT TransitionStartFrame = 86;
constexpr UINT TransitionEndFrame = 109;
constexpr UINT IdleReturnFrame = 71;
constexpr UINT TouchBounceFrameCount = 6;
constexpr UINT AlarmHandGatherMs = 100;
constexpr float AlarmHandTurn = 1.0f / 12.0f;

#include "clock-dial.h"
#include "clock-animation.h"
#include "clock-cheek-warp.h"

struct ClockHand {
    std::unique_ptr<Bitmap> image;
    PointF center, tip;
    float nativeAngle = 0;
};
std::unique_ptr<AnimationImage> gifImage;
std::unique_ptr<AnimationImage> transitionImage;
std::unique_ptr<AnimationImage> smashImage;
std::unique_ptr<AnimationImage> angryImage;
std::unique_ptr<AnimationImage> angryAltImage;
std::unique_ptr<AnimationImage> panicImage;
std::unique_ptr<AnimationImage> blankImage;
std::unique_ptr<AnimationImage> patIdleImage;
std::unique_ptr<AnimationImage> patEndImage;
std::unique_ptr<AnimationImage> touchBackImage;
std::unique_ptr<AnimationImage> touchHeadImage;
std::unique_ptr<AnimationImage> touchFrontImage;
std::unique_ptr<AnimationImage> touchEndBackImage;
std::unique_ptr<AnimationImage> touchEndHeadImage;
std::unique_ptr<AnimationImage> touchEndFrontImage;
std::unique_ptr<AnimationImage> touchEndImage;
std::array<AnimationImage*, 16> ArtworkAnimations() {
    return { gifImage.get(), transitionImage.get(), smashImage.get(), angryImage.get(), angryAltImage.get(),
        panicImage.get(), blankImage.get(), patIdleImage.get(), patEndImage.get(), touchBackImage.get(),
        touchHeadImage.get(), touchFrontImage.get(), touchEndBackImage.get(), touchEndHeadImage.get(),
        touchEndFrontImage.get(), touchEndImage.get() };
}
ClockHand clockHands[2];
PointF clockCenter(120, 120);
PointF smashOffset, angryOffset, angryAltOffset;
PointF panicOffset;
PointF blankOffset;
PointF touchHeadOffset, touchFrontOffset;
PointF touchEndHeadOffset, touchEndFrontOffset;
PointF cheekTouchSource;
PointF cheekPull;
PointF patPull;
PointF touchReleasePullSource;
PointF touchReleaseWarpCenter;
float touchWarpBaseRadius = 0;
int artworkWidth = 240, artworkHeight = 240;
std::vector<UINT> frameEnds;
std::vector<UINT> transitionSourceFrameEnds;
std::vector<UINT> transitionFrameEnds;
std::vector<UINT> smashFrameEnds;
std::vector<UINT> angryFrameEnds;
std::vector<UINT> angryAltSourceFrameEnds;
std::vector<UINT> angryAltFrameEnds;
std::vector<UINT> panicFrameEnds;
std::vector<UINT> blankFrameEnds;
std::vector<UINT> patIdleFrameEnds;
std::vector<UINT> patEndFrameEnds;
std::vector<UINT> touchFrameEnds;
std::vector<UINT> touchEndFrameEnds;
std::vector<PointF> frameCenters;
std::vector<PointF> touchWarpCenters;
UINT activeFrame = 0;
ULONGLONG animationStart = 0;
ULONGLONG idlePhaseOffset = 0;
bool reactionActive = false;
bool showingAngry = false;
bool showingAngryAlt = false;
bool showingTransition = false;
bool handReturnActive = false;
bool cheekDragging = false;
bool touchReleaseActive = false;
bool alarmActive = false;
bool alarmPanicActive = false;
bool blankPlaybackActive = false;
bool patActive = false;
bool patReleaseActive = false;
ULONGLONG handReturnStart = 0;
float handReturnStartTurns[2] = { .5f, .5f };
float alarmHandStartTurns[2] = {};
float handReturnStartLengthScales[2] = { 1.0f, 1.0f };
float alarmHandStartLengthScales[2] = { 1.0f, 1.0f };

bool ReadFrameEnds(AnimationImage& image, std::vector<UINT>& ends) {
    ends = image.frameEnds;
    return !ends.empty();
}

void BuildTransitionFrameEnds() {
    transitionFrameEnds.clear();
    const UINT sourceStart = TransitionStartFrame ? transitionSourceFrameEnds[TransitionStartFrame - 1] : 0;
    for (UINT i = TransitionStartFrame; i <= TransitionEndFrame; ++i) {
        const UINT elapsed = transitionSourceFrameEnds[i] - sourceStart;
        transitionFrameEnds.push_back(static_cast<UINT>(std::lround(elapsed / .75f)));
    }
}

void BuildAngryAltFrameEnds() {
    angryAltFrameEnds.clear();
    UINT sourceStart = angryAltSourceFrameEnds[AngryAltStartFrame - 1], duration = 0;
    const UINT lastFrame = static_cast<UINT>(angryAltSourceFrameEnds.size() - 1);
    for (UINT i = AngryAltStartFrame; i <= lastFrame; ++i) {
        const UINT sourceDelay = angryAltSourceFrameEnds[i] - sourceStart;
        sourceStart = angryAltSourceFrameEnds[i];
        const float progress = (i - AngryAltStartFrame) / static_cast<float>(lastFrame - AngryAltStartFrame);
        const float speed = 1.0f - progress * .5f;
        duration += static_cast<UINT>(std::lround(sourceDelay / speed));
        angryAltFrameEnds.push_back(duration);
    }
}

void ReleaseArtwork() {
    cheekWarpBuffers.Release();
    clockDial.reset();
    for (auto& hand : clockHands) hand.image.reset();
    gifImage.reset();
    transitionImage.reset();
    smashImage.reset();
    angryImage.reset();
    angryAltImage.reset();
    panicImage.reset();
    blankImage.reset();
    patIdleImage.reset();
    patEndImage.reset();
    touchBackImage.reset();
    touchHeadImage.reset();
    touchFrontImage.reset();
    touchEndBackImage.reset();
    touchEndHeadImage.reset();
    touchEndFrontImage.reset();
    touchEndImage.reset();
    frameEnds.clear();
    transitionSourceFrameEnds.clear();
    transitionFrameEnds.clear();
    smashFrameEnds.clear();
    angryFrameEnds.clear();
    angryAltSourceFrameEnds.clear();
    angryAltFrameEnds.clear();
    panicFrameEnds.clear();
    blankFrameEnds.clear();
    patIdleFrameEnds.clear();
    patEndFrameEnds.clear();
    touchFrameEnds.clear();
    touchEndFrameEnds.clear();
    frameCenters.clear();
    touchWarpCenters.clear();
    idlePhaseOffset = 0;
    reactionActive = false;
    showingAngry = false;
    showingAngryAlt = false;
    showingTransition = false;
    handReturnActive = false;
    cheekDragging = false;
    touchReleaseActive = false;
    alarmActive = false;
    alarmPanicActive = false;
    blankPlaybackActive = false;
    patActive = patReleaseActive = false;
    smashOffset = PointF();
    angryOffset = PointF();
    angryAltOffset = PointF();
    panicOffset = PointF();
    blankOffset = PointF();
    touchHeadOffset = PointF();
    touchFrontOffset = PointF();
    touchEndHeadOffset = PointF();
    touchEndFrontOffset = PointF();
    cheekTouchSource = PointF();
    cheekPull = PointF();
    patPull = PointF();
    touchReleasePullSource = PointF();
    touchReleaseWarpCenter = PointF();
    touchWarpBaseRadius = 0;
}

bool LoadArtwork(const std::wstring& directory) {
    const std::wstring config = directory + L"locate data\\clock-layout.ini";
    auto read = [&](const wchar_t* section, const wchar_t* key) {
        return static_cast<int>(GetPrivateProfileIntW(section, key, -1, config.c_str()));
    };
    auto readString = [&](const wchar_t* section, const wchar_t* key) {
        wchar_t value[MAX_PATH] = {};
        GetPrivateProfileStringW(section, key, L"", value, MAX_PATH, config.c_str());
        return std::wstring(value);
    };
    auto readFloat = [&](const wchar_t* section, const wchar_t* key, float missing = -1.0f) {
        float value = missing;
        swscanf_s(readString(section, key).c_str(), L"%f", &value);
        return value;
    };
    auto point = [&](const wchar_t* section, const wchar_t* x, const wchar_t* y) {
        return PointF(readFloat(section, x) + .5f, readFloat(section, y) + .5f);
    };
    auto valid = [](PointF p, auto& image) {
        return p.X >= 0 && p.Y >= 0 && p.X < image.GetWidth() && p.Y < image.GetHeight();
    };
    auto loadImage = [&](const wchar_t* section, std::unique_ptr<AnimationImage>& image) {
        const std::wstring file = readString(section, L"file");
        if (file.empty()) return false;
        image = std::make_unique<AnimationImage>();
        return image->Load(directory, config, section) &&
            read(section, L"width") == static_cast<int>(image->GetWidth()) &&
            read(section, L"height") == static_cast<int>(image->GetHeight());
    };
    if (!loadImage(L"background", gifImage) || !loadImage(L"touch", transitionImage)) return false;
    cheekTouchSource = point(L"touch", L"x", L"y");
    if (!valid(cheekTouchSource, *transitionImage)) return false;
    auto loadAligned = [&](const wchar_t* section, std::unique_ptr<AnimationImage>& image, PointF& offset) {
        const float x = readFloat(section, L"offsetX", static_cast<float>(INT_MAX));
        const float y = readFloat(section, L"offsetY", static_cast<float>(INT_MAX));
        if (x == static_cast<float>(INT_MAX) || y == static_cast<float>(INT_MAX) || !loadImage(section, image)) return false;
        offset = PointF(static_cast<float>(x), static_cast<float>(y));
        return true;
    };
    if (!loadAligned(L"smash", smashImage, smashOffset) || !loadAligned(L"angry", angryImage, angryOffset) ||
        !loadAligned(L"angry_alt", angryAltImage, angryAltOffset) ||
        !loadAligned(L"panic", panicImage, panicOffset) ||
        !loadAligned(L"blank", blankImage, blankOffset) ||
        !loadImage(L"pat_idle", patIdleImage) || !loadImage(L"pat_end", patEndImage) ||
        !loadImage(L"touch_back", touchBackImage) ||
        !loadAligned(L"touch_head", touchHeadImage, touchHeadOffset) ||
        !loadAligned(L"touch_front", touchFrontImage, touchFrontOffset) ||
        !loadImage(L"touch_end_back", touchEndBackImage) ||
        !loadImage(L"touch_end", touchEndImage) ||
        !loadAligned(L"touch_end_head", touchEndHeadImage, touchEndHeadOffset) ||
        !loadAligned(L"touch_end_front", touchEndFrontImage, touchEndFrontOffset)) return false;
    const int touchWarpFrameCount = read(L"touch_head", L"frameCount");
    touchWarpBaseRadius = readFloat(L"touch_head", L"radius");
    if (touchWarpFrameCount <= 0 || touchWarpBaseRadius <= 0 ||
        touchWarpFrameCount != static_cast<int>(touchHeadImage->GetFrameCount())) return false;
    touchWarpCenters.clear();
    for (int i = 0; i < touchWarpFrameCount; ++i) {
        const std::wstring key = L"frame" + std::to_wstring(i);
        wchar_t value[64] = {};
        GetPrivateProfileStringW(L"touch_head", key.c_str(), L"", value, 64, config.c_str());
        float x = -1, y = -1;
        if (swscanf_s(value, L"%f,%f", &x, &y) != 2) return false;
        PointF center(x + .5f, y + .5f);
        if (!valid(center, *touchHeadImage)) return false;
        touchWarpCenters.push_back(center);
    }
    const int frameCount = read(L"background", L"frameCount");
    if (read(L"background", L"width") != static_cast<int>(gifImage->GetWidth()) ||
        read(L"background", L"height") != static_cast<int>(gifImage->GetHeight()) || frameCount <= 0 ||
        frameCount != static_cast<int>(gifImage->GetFrameCount())) return false;

    frameCenters.clear();
    float left = -DialRadius, top = -DialRadius, right = DialRadius, bottom = DialRadius;
    for (int i = 0; i < frameCount; ++i) {
        const std::wstring key = L"frame" + std::to_wstring(i);
        wchar_t value[64] = {};
        GetPrivateProfileStringW(L"background", key.c_str(), L"", value, 64, config.c_str());
        float x = -1, y = -1;
        if (swscanf_s(value, L"%f,%f", &x, &y) != 2) return false;
        PointF center(x + .5f, y + .5f);
        if (!valid(center, *gifImage)) return false;
        frameCenters.push_back(center);
    }

    const wchar_t* roles[] = { L"hour", L"minute" };
    for (int i = 0; i < 2; ++i) {
        auto& hand = clockHands[i];
        const std::wstring file = readString(roles[i], L"file");
        if (file.empty()) return false;
        hand.image.reset(Bitmap::FromFile((directory + file).c_str()));
        if (!hand.image || hand.image->GetLastStatus() != Ok ||
            read(roles[i], L"width") != static_cast<int>(hand.image->GetWidth()) ||
            read(roles[i], L"height") != static_cast<int>(hand.image->GetHeight())) return false;
        hand.center = point(roles[i], L"centerX", L"centerY");
        hand.tip = point(roles[i], L"tipX", L"tipY");
        if (!valid(hand.center, *hand.image) || !valid(hand.tip, *hand.image)) return false;
        const float dx = hand.tip.X - hand.center.X, dy = hand.tip.Y - hand.center.Y;
        if (dx == 0 && dy == 0) return false;
        hand.nativeAngle = std::atan2(dx, -dy) * 180 / Pi;
        const float radius = std::hypot(std::max(hand.center.X, hand.image->GetWidth() - hand.center.X),
            std::max(hand.center.Y, hand.image->GetHeight() - hand.center.Y)) * HandScale;
        left = std::min(left, -radius);
        top = std::min(top, -radius);
        right = std::max(right, radius);
        bottom = std::max(bottom, radius);
    }
    constexpr float margin = 12;
    clockCenter = PointF(margin - left, margin - top);
    artworkWidth = static_cast<int>(std::ceil(right - left + margin * 2));
    artworkHeight = static_cast<int>(std::ceil(bottom - top + margin * 2));
#ifdef SKEA_TEST_BUILD
    artworkWidth += TestPanelWidth;
    clockCenter.X += TestPanelWidth;
#endif

    std::vector<UINT> touchHeadFrameEnds, touchFrontFrameEnds;
    std::vector<UINT> touchEndBackFrameEnds, touchEndHeadFrameEnds, touchEndFrontFrameEnds;
    if (!ReadFrameEnds(*gifImage, frameEnds) || !ReadFrameEnds(*transitionImage, transitionSourceFrameEnds) ||
        !ReadFrameEnds(*smashImage, smashFrameEnds) || !ReadFrameEnds(*angryImage, angryFrameEnds) ||
        !ReadFrameEnds(*angryAltImage, angryAltSourceFrameEnds) || !ReadFrameEnds(*panicImage, panicFrameEnds) ||
        !ReadFrameEnds(*blankImage, blankFrameEnds) ||
        !ReadFrameEnds(*patIdleImage, patIdleFrameEnds) || !ReadFrameEnds(*patEndImage, patEndFrameEnds) ||
        !ReadFrameEnds(*touchBackImage, touchFrameEnds) ||
        !ReadFrameEnds(*touchHeadImage, touchHeadFrameEnds) || !ReadFrameEnds(*touchFrontImage, touchFrontFrameEnds) ||
        !ReadFrameEnds(*touchEndImage, touchEndFrameEnds) ||
        !ReadFrameEnds(*touchEndBackImage, touchEndBackFrameEnds) ||
        !ReadFrameEnds(*touchEndHeadImage, touchEndHeadFrameEnds) ||
        !ReadFrameEnds(*touchEndFrontImage, touchEndFrontFrameEnds) ||
        touchHeadFrameEnds != touchFrameEnds || touchFrontFrameEnds != touchFrameEnds ||
        touchEndHeadFrameEnds != touchEndFrameEnds || touchEndFrontFrameEnds != touchEndFrameEnds ||
        touchEndBackFrameEnds != touchEndFrameEnds ||
        touchEndImage->GetWidth() != touchEndBackImage->GetWidth() ||
        touchEndImage->GetHeight() != touchEndBackImage->GetHeight() ||
        touchEndFrameEnds.size() <= TouchBounceFrameCount ||
        touchWarpCenters.size() != touchFrameEnds.size() ||
        smashFrameEnds.size() <= SmashStartFrame || transitionSourceFrameEnds.size() <= TransitionEndFrame ||
        angryFrameEnds.size() <= AngryCutFrame || angryAltSourceFrameEnds.size() <= AngryAltStartFrame ||
        frameEnds.size() <= IdleReturnFrame) return false;
    BuildTransitionFrameEnds();
    BuildAngryAltFrameEnds();
    activeFrame = 0;
    animationStart = GetTickCount64();
    idlePhaseOffset = 0;
    reactionActive = false;
    showingAngry = false;
    showingAngryAlt = false;
    showingTransition = false;
    handReturnActive = false;
    cheekDragging = false;
    touchReleaseActive = false;
    alarmActive = false;
    alarmPanicActive = false;
    blankPlaybackActive = false;
    patActive = patReleaseActive = false;
    cheekPull = PointF();
    touchReleasePullSource = PointF();
    touchReleaseWarpCenter = PointF();
    clockDial = std::make_unique<ClockDial>();
    if (!clockDial->Initialize(clockCenter)) return false;
    for (AnimationImage* image : ArtworkAnimations()) {
        if (!image->Preload(image == gifImage.get() ? IdleReturnFrame : UINT_MAX)) return false;
    }
    return gifImage->SelectFrame(0) == Ok &&
        transitionImage->SelectFrame(TransitionStartFrame) == Ok &&
        smashImage->SelectFrame(SmashStartFrame) == Ok &&
        angryImage->SelectFrame(0) == Ok &&
        angryAltImage->SelectFrame(AngryAltStartFrame) == Ok &&
        panicImage->SelectFrame(0) == Ok &&
        blankImage->SelectFrame(0) == Ok &&
        patIdleImage->SelectFrame(0) == Ok &&
        patEndImage->SelectFrame(0) == Ok &&
        touchBackImage->SelectFrame(0) == Ok &&
        touchHeadImage->SelectFrame(0) == Ok &&
        touchFrontImage->SelectFrame(0) == Ok &&
        touchEndBackImage->SelectFrame(0) == Ok &&
        touchEndHeadImage->SelectFrame(0) == Ok &&
        touchEndImage->SelectFrame(TouchBounceFrameCount) == Ok &&
        touchEndFrontImage->SelectFrame(0) == Ok && gifImage->Frame() != nullptr;
}

UINT FrameAt(const std::vector<UINT>& ends, ULONGLONG elapsed) {
    const UINT phase = static_cast<UINT>(elapsed % ends.back());
    return static_cast<UINT>(std::upper_bound(ends.begin(), ends.end(), phase) - ends.begin());
}

enum class ReactionStage { Smash, Angry, AngryAlt, Transition, Finished };

UINT SmashStartOffset() {
    return SmashStartFrame ? smashFrameEnds[SmashStartFrame - 1] : 0;
}

ULONGLONG AngryStart() {
    return smashFrameEnds.back() - SmashStartOffset();
}

ULONGLONG AngryAltStart() {
    return AngryStart() + static_cast<ULONGLONG>(angryFrameEnds.back()) * 2 + angryFrameEnds[AngryCutFrame];
}

ULONGLONG TransitionStart() {
    return AngryAltStart() + angryAltFrameEnds.back();
}

ULONGLONG ReactionDuration() {
    return TransitionStart() + transitionFrameEnds.back();
}

ReactionStage ReactionStageAt(ULONGLONG elapsed) {
    if (elapsed < AngryStart()) return ReactionStage::Smash;
    if (elapsed < AngryAltStart()) return ReactionStage::Angry;
    if (elapsed < TransitionStart()) return ReactionStage::AngryAlt;
    if (elapsed < ReactionDuration()) return ReactionStage::Transition;
    return ReactionStage::Finished;
}

PointF GifPosition() {
    return PointF(clockCenter.X - frameCenters[0].X * CharacterScale,
        clockCenter.Y - frameCenters[0].Y * CharacterScale);
}

PointF CheekTouchCenter() {
    const PointF idle = GifPosition();
    return PointF(idle.X + cheekTouchSource.X * CharacterScale,
        idle.Y + cheekTouchSource.Y * CharacterScale);
}

UINT TouchWarpFrame() {
    return cheekDragging && activeFrame < touchWarpCenters.size() ? activeFrame : 0;
}

PointF TouchWarpCenterSource() {
    const PointF point = touchWarpCenters[TouchWarpFrame()];
    return PointF(touchHeadOffset.X + point.X, touchHeadOffset.Y + point.Y);
}

PointF ClampedCheekPull(PointF mouse) {
    const PointF center = CheekTouchCenter();
    float dx = mouse.X - center.X, dy = mouse.Y - center.Y;
    const float distance = std::hypot(dx, dy);
    const float limit = CheekTouchRadius * 1.5f;
    if (distance > limit) {
        dx *= limit / distance;
        dy *= limit / distance;
    }
    return PointF(dx, dy);
}

AnimationImage* CurrentGif() {
    if (touchReleaseActive && activeFrame >= TouchBounceFrameCount) return touchEndImage.get();
    if (alarmActive && alarmPanicActive) return panicImage.get();
    if (blankPlaybackActive) return blankImage.get();
    if (patActive) return patIdleImage.get();
    if (patReleaseActive) return patEndImage.get();
    if (!reactionActive) return gifImage.get();
    if (showingTransition) return transitionImage.get();
    if (showingAngryAlt) return angryAltImage.get();
    return showingAngry ? angryImage.get() : smashImage.get();
}

bool HandsVisible() {
    if (blankPlaybackActive) return false;
    if (alarmActive) return !alarmPanicActive;
    return !cheekDragging && !touchReleaseActive && (!reactionActive || showingTransition);
}

float HandAnimationProgress(float progress) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    return progress * progress * (3 - 2 * progress);
}

float HandLengthScale(int index, ULONGLONG tick) {
    if (!HandsVisible()) return 1.0f;
    float start = 1.0f, target = TimeHandLengthScales[index], progress = 1.0f;
    if (alarmActive && !alarmPanicActive) {
        start = alarmHandStartLengthScales[index];
        target = 1.0f;
        progress = (tick - animationStart) / static_cast<float>(AlarmHandGatherMs);
    } else if (handReturnActive) {
        start = handReturnStartLengthScales[index];
        progress = (tick - handReturnStart) / static_cast<float>(HandReturnMs);
    }
    return start + (target - start) * HandAnimationProgress(progress);
}

void BeginAlarm(const SYSTEMTIME& now) {
    const ULONGLONG tick = GetTickCount64();
    for (int i = 0; i < 2; ++i) alarmHandStartLengthScales[i] = HandLengthScale(i, tick);
    cheekWarpBuffers.Release();
    blankPlaybackActive = false;
    patActive = patReleaseActive = false;
    reactionActive = false;
    showingAngry = false;
    showingAngryAlt = false;
    showingTransition = false;
    cheekDragging = false;
    touchReleaseActive = false;
    cheekPull = PointF();
    handReturnActive = false;
    alarmActive = true;
    alarmPanicActive = false;
    alarmHandStartTurns[0] = HourTurn(now.wHour, now.wMinute, now.wSecond);
    alarmHandStartTurns[1] = MinuteTurn(now.wMinute, now.wSecond);
    activeFrame = 0;
    animationStart = tick;
    idlePhaseOffset = 0;
    gifImage->SelectFrame(0);
}

void BeginBlankPlayback() {
    cheekWarpBuffers.Release();
    patActive = patReleaseActive = false;
    alarmActive = false;
    alarmPanicActive = false;
    reactionActive = false;
    showingAngry = false;
    showingAngryAlt = false;
    showingTransition = false;
    cheekDragging = false;
    touchReleaseActive = false;
    cheekPull = PointF();
    handReturnActive = false;
    blankPlaybackActive = true;
    activeFrame = 0;
    animationStart = GetTickCount64();
    blankImage->SelectFrame(0);
}

void EndBlankPlayback() {
    if (!blankPlaybackActive) return;
    blankPlaybackActive = false;
    activeFrame = 0;
    animationStart = GetTickCount64();
    idlePhaseOffset = 0;
    gifImage->SelectFrame(0);
}

void DismissAlarmToIdle() {
    if (!alarmActive) return;
    const ULONGLONG tick = GetTickCount64();
    for (int i = 0; i < 2; ++i) handReturnStartLengthScales[i] = HandLengthScale(i, tick);
    alarmActive = false;
    alarmPanicActive = false;
    animationStart = tick;
    idlePhaseOffset = 0;
    activeFrame = 0;
    gifImage->SelectFrame(0);
    handReturnActive = true;
    handReturnStartTurns[0] = handReturnStartTurns[1] = AlarmHandTurn;
    handReturnStart = animationStart;
}

void BeginReaction() {
    cheekWarpBuffers.Release();
    patActive = patReleaseActive = false;
    blankPlaybackActive = false;
    alarmActive = false;
    alarmPanicActive = false;
    cheekDragging = false;
    touchReleaseActive = false;
    cheekPull = PointF();
    reactionActive = true;
    showingAngry = false;
    showingAngryAlt = false;
    showingTransition = false;
    handReturnActive = false;
    activeFrame = SmashStartFrame;
    animationStart = GetTickCount64();
    smashImage->SelectFrame(SmashStartFrame);
}

void BeginCheekPull() {
    cheekWarpBuffers.Release();
    patActive = patReleaseActive = false;
    blankPlaybackActive = false;
    alarmActive = false;
    alarmPanicActive = false;
    reactionActive = false;
    showingAngry = false;
    showingAngryAlt = false;
    showingTransition = false;
    handReturnActive = false;
    cheekDragging = true;
    touchReleaseActive = false;
    cheekPull = PointF();
    activeFrame = 0;
    animationStart = GetTickCount64();
    touchBackImage->SelectFrame(0);
    touchHeadImage->SelectFrame(0);
    touchFrontImage->SelectFrame(0);
}

void BeginPat() {
    cheekWarpBuffers.Release();
    patPull = PointF();
    blankPlaybackActive = false;
    alarmActive = alarmPanicActive = false;
    reactionActive = showingAngry = showingAngryAlt = showingTransition = false;
    cheekDragging = touchReleaseActive = false;
    cheekPull = PointF();
    handReturnActive = false;
    patActive = true;
    patReleaseActive = false;
    activeFrame = 0;
    animationStart = GetTickCount64();
    idlePhaseOffset = 0;
    patIdleImage->SelectFrame(0);
}

void EndPat() {
    if (!patActive) return;
    cheekWarpBuffers.Release();
    patPull = PointF();
    patActive = false;
    patReleaseActive = true;
    activeFrame = 0;
    animationStart = GetTickCount64();
    patEndImage->SelectFrame(0);
}

PointF EffectiveCheekWarpPull();

bool SelectLayerFrames(UINT frame, AnimationImage& back, AnimationImage& head, AnimationImage& front) {
    const bool backReady = back.Ready(frame), headReady = head.Ready(frame), frontReady = front.Ready(frame);
    if (!backReady || !headReady || !frontReady) return false;
    return back.SelectFrame(frame) == Ok && head.SelectFrame(frame) == Ok && front.SelectFrame(frame) == Ok;
}

bool SelectTouchReleaseFrame(UINT frame) {
    if (frame >= TouchBounceFrameCount) {
        if (touchEndImage->SelectFrame(frame) != Ok) return false;
        cheekWarpBuffers.Release();
        return true;
    }
    return SelectLayerFrames(frame, *touchEndBackImage, *touchEndHeadImage, *touchEndFrontImage);
}

void EndCheekPull() {
    if (!cheekDragging) return;
    const PointF headPoint = touchWarpCenters[TouchWarpFrame()];
    touchReleaseWarpCenter = PointF(touchEndHeadOffset.X + headPoint.X,
        touchEndHeadOffset.Y + headPoint.Y);
    touchReleasePullSource = EffectiveCheekWarpPull();
    cheekPull = PointF();
    cheekDragging = false;
    touchReleaseActive = true;
    activeFrame = 0;
    animationStart = GetTickCount64();
    handReturnActive = false;
    touchEndBackImage->SelectFrame(0);
    touchEndHeadImage->SelectFrame(0);
    touchEndFrontImage->SelectFrame(0);
}

float TouchReleasePullScale(UINT frame) {
    static constexpr float scales[TouchBounceFrameCount] = {
        1.0f, .55f, .08f, -.14f, .07f, 0.0f
    };
    return frame < TouchBounceFrameCount ? scales[frame] : 0.0f;
}

void CompleteTouchRelease(ULONGLONG now) {
    cheekWarpBuffers.Release();
    touchReleaseActive = false;
    cheekPull = PointF();
    touchReleasePullSource = PointF();
    animationStart = now;
    idlePhaseOffset = 0;
    activeFrame = 0;
    handReturnActive = true;
    handReturnStartTurns[0] = handReturnStartTurns[1] = .5f;
    handReturnStartLengthScales[0] = handReturnStartLengthScales[1] = 1.0f;
    handReturnStart = now;
    gifImage->SelectFrame(0);
}

PointF AnimationPosition(AnimationImage& image) {
    const PointF idle = GifPosition();
    PointF offset;
    if (&image == smashImage.get()) offset = smashOffset;
    else if (&image == angryImage.get()) offset = angryOffset;
    else if (&image == angryAltImage.get()) offset = angryAltOffset;
    else if (&image == panicImage.get()) offset = panicOffset;
    else if (&image == blankImage.get()) offset = blankOffset;
    return PointF(idle.X + offset.X * CharacterScale, idle.Y + offset.Y * CharacterScale);
}

void HandTransform(Matrix& transform, const ClockHand& hand, float turns, PointF center) {
    transform.Translate(center.X, center.Y);
    transform.Rotate(turns * 360 - hand.nativeAngle);
    transform.Scale(HandScale, HandScale);
    transform.Translate(-hand.center.X, -hand.center.Y);
}

PointF IdleHandCenterAt(float frame) {
    if (frameCenters.empty()) return clockCenter;
    frame = std::clamp(frame, 0.0f, static_cast<float>(frameCenters.size() - 1));
    const size_t first = static_cast<size_t>(std::floor(frame));
    const size_t second = std::min(first + 1, frameCenters.size() - 1);
    const float blend = frame - first;
    const PointF center(
        frameCenters[first].X + (frameCenters[second].X - frameCenters[first].X) * blend,
        frameCenters[first].Y + (frameCenters[second].Y - frameCenters[first].Y) * blend);
    const PointF position = GifPosition();
    return PointF(position.X + center.X * CharacterScale,
        position.Y + center.Y * CharacterScale);
}

PointF IdleHandCenter(UINT frame) {
    return IdleHandCenterAt(static_cast<float>(frame));
}

float TransitionIdleFrame(UINT frame) {
    if (transitionSourceFrameEnds.size() < 2 || frameCenters.size() < 2) return 0;
    return frame * static_cast<float>(frameCenters.size() - 1) /
        static_cast<float>(transitionSourceFrameEnds.size() - 1);
}

PointF TransitionHandCenter(UINT frame) {
    return IdleHandCenterAt(TransitionIdleFrame(frame));
}

void StretchedHandTransform(Matrix& transform, const ClockHand& hand,
    PointF movingCenter, PointF fixedTip) {
    const float vx = hand.tip.X - hand.center.X;
    const float vy = hand.tip.Y - hand.center.Y;
    const float sourceLength = std::hypot(vx, vy);
    const float dx = fixedTip.X - movingCenter.X;
    const float dy = fixedTip.Y - movingCenter.Y;
    const float targetLength = std::hypot(dx, dy);
    if (sourceLength <= .001f || targetLength <= .001f) {
        transform.SetElements(HandScale, 0, 0, HandScale,
            movingCenter.X - hand.center.X * HandScale,
            movingCenter.Y - hand.center.Y * HandScale);
        return;
    }

    const float ux = vx / sourceLength;
    const float uy = vy / sourceLength;
    const float ax = dx / sourceLength;
    const float ay = dy / sourceLength;
    const float bx = -dy / targetLength * HandScale;
    const float by = dx / targetLength * HandScale;
    const float m11 = ax * ux - bx * uy;
    const float m12 = ay * ux - by * uy;
    const float m21 = ax * uy + bx * ux;
    const float m22 = ay * uy + by * ux;
    transform.SetElements(m11, m12, m21, m22,
        movingCenter.X - m11 * hand.center.X - m21 * hand.center.Y,
        movingCenter.Y - m12 * hand.center.X - m22 * hand.center.Y);
}

float AnimatedHandTurn(float start, float target, float progress) {
    const float eased = HandAnimationProgress(progress);
    const float shortest = std::fmod(target - start + 1.5f, 1.0f) - 0.5f;
    return start + shortest * eased;
}

bool ShouldDrawTick(int index, bool drawTicks, bool drawNumbers) {
    return drawTicks && !(drawNumbers && index % 5 == 0);
}

PointF EffectiveCheekWarpPull() {
    PointF pull(cheekPull.X / CharacterScale, cheekPull.Y / CharacterScale);
    const float inwardLimit = touchWarpBaseRadius * MaxInwardCheekPull;
    if (pull.X > inwardLimit) {
        const float scale = inwardLimit / pull.X;
        pull.X *= scale;
        pull.Y *= scale;
    }
    return pull;
}

Bitmap* WarpedHead(UINT width, UINT height, Bitmap& head, PointF headOffset,
    PointF warpCenter, PointF effectivePull, float warpRadius, bool radial) {
    if (!cheekWarpBuffers.Prepare(width, height, head, headOffset, activeFrame)) return nullptr;
    Bitmap* source = cheekWarpBuffers.source.get();
    Bitmap* result = cheekWarpBuffers.result.get();
    float pullX = effectivePull.X, pullY = effectivePull.Y;
    if (std::hypot(pullX, pullY) < .01f) return source;
    const float warpCenterX = warpCenter.X - .5f, warpCenterY = warpCenter.Y - .5f;
    {
        Graphics graphics(result);
        graphics.Clear(Color(0, 0, 0, 0));
        graphics.SetCompositingMode(CompositingModeSourceCopy);
        graphics.DrawImage(source, 0, 0, width, height);
    }
    Rect bounds(0, 0, width, height);
    BitmapData sourceData = {}, resultData = {};
    if (source->LockBits(&bounds, ImageLockModeRead, PixelFormat32bppPARGB, &sourceData) != Ok ||
        result->LockBits(&bounds, ImageLockModeWrite, PixelFormat32bppPARGB, &resultData) != Ok) {
        if (sourceData.Scan0) source->UnlockBits(&sourceData);
        return nullptr;
    }
    auto pixel = [&](int x, int y, int channel) -> float {
        if (x < 0 || y < 0 || x >= static_cast<int>(width) || y >= static_cast<int>(height)) return 0;
        const BYTE* row = static_cast<const BYTE*>(sourceData.Scan0) + y * sourceData.Stride;
        return row[x * 4 + channel];
    };
    if (!radial && (pullX <= 0 || std::abs(pullY) > .01f)) {
        for (int y = std::max(0, static_cast<int>(std::floor(warpCenterY - warpRadius)));
            y <= std::min(static_cast<int>(height) - 1, static_cast<int>(std::ceil(warpCenterY + warpRadius))); ++y) {
            const float vertical = (y - warpCenterY) / warpRadius;
            if (std::abs(vertical) >= 1) continue;
            const float chord = std::sqrt(1 - vertical * vertical);
            const int first = std::max(0, static_cast<int>(std::floor(warpCenterX - warpRadius * chord)));
            const int last = std::min(static_cast<int>(width) - 1,
                static_cast<int>(std::floor(warpCenterX + warpRadius * chord - CheekSeamOverlap)));
            BYTE* output = static_cast<BYTE*>(resultData.Scan0) + y * resultData.Stride;
            for (int x = first; x <= last; ++x)
                output[x * 4] = output[x * 4 + 1] = output[x * 4 + 2] = output[x * 4 + 3] = 0;
        }
    }
    struct WarpVertex { float x, y, sourceX, sourceY; };
    constexpr int Rows = CheekRows, Columns = CheekColumns;
    WarpVertex mesh[Rows + 1][Columns + 1] = {};
    cheekWarpBuffers.PrepareGeometry(warpCenter, warpRadius);
    for (int row = 0; row <= Rows; ++row) {
        for (int column = 0; column <= Columns; ++column) {
            const PointF point = cheekWarpBuffers.original[row][column];
            float weight = cheekWarpGrid.weight[row][column];
            if (radial) {
                const float dx = (point.X - warpCenterX) / warpRadius;
                const float dy = (point.Y - warpCenterY) / warpRadius;
                const float falloff = std::max(0.0f, 1 - dx * dx - dy * dy);
                weight = falloff * falloff;
            }
            mesh[row][column] = { point.X + pullX * weight, point.Y + pullY * weight, point.X, point.Y };
        }
    }
    auto edge = [](const WarpVertex& a, const WarpVertex& b, float x, float y) {
        return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
    };
    auto drawTriangle = [&](const WarpVertex& a, const WarpVertex& b, const WarpVertex& c) {
        const float area = edge(a, b, c.x, c.y);
        if (std::abs(area) < .001f) return;
        const int firstX = std::max(0, static_cast<int>(std::floor(std::min({ a.x, b.x, c.x }))));
        const int firstY = std::max(0, static_cast<int>(std::floor(std::min({ a.y, b.y, c.y }))));
        const int lastX = std::min(static_cast<int>(width) - 1,
            static_cast<int>(std::ceil(std::max({ a.x, b.x, c.x }))));
        const int lastY = std::min(static_cast<int>(height) - 1,
            static_cast<int>(std::ceil(std::max({ a.y, b.y, c.y }))));
        for (int y = firstY; y <= lastY; ++y) {
            BYTE* output = static_cast<BYTE*>(resultData.Scan0) + y * resultData.Stride;
            for (int x = firstX; x <= lastX; ++x) {
                const float px = static_cast<float>(x), py = static_cast<float>(y);
                const float wa = edge(b, c, px, py) / area;
                const float wb = edge(c, a, px, py) / area;
                const float wc = 1 - wa - wb;
                if (wa < -.001f || wb < -.001f || wc < -.001f) continue;
                const float sourceX = a.sourceX * wa + b.sourceX * wb + c.sourceX * wc;
                const float sourceY = a.sourceY * wa + b.sourceY * wb + c.sourceY * wc;
                const int left = static_cast<int>(std::floor(sourceX));
                const int top = static_cast<int>(std::floor(sourceY));
                const float fx = sourceX - left, fy = sourceY - top;
                float sample[4];
                for (int channel = 0; channel < 4; ++channel) {
                    const float upper = pixel(left, top, channel) * (1 - fx) + pixel(left + 1, top, channel) * fx;
                    const float lower = pixel(left, top + 1, channel) * (1 - fx) + pixel(left + 1, top + 1, channel) * fx;
                    sample[channel] = std::clamp(upper * (1 - fy) + lower * fy, 0.0f, 255.0f);
                }
                const float remaining = radial ? 0.0f : 1 - sample[3] / 255.0f;
                for (int channel = 0; channel < 4; ++channel)
                    output[x * 4 + channel] = static_cast<BYTE>(std::clamp(
                        sample[channel] + output[x * 4 + channel] * remaining, 0.0f, 255.0f));
            }
        }
    };
    for (int row = 0; row < Rows; ++row) for (int column = 0; column < Columns; ++column) {
        drawTriangle(mesh[row][column], mesh[row + 1][column], mesh[row + 1][column + 1]);
        drawTriangle(mesh[row][column], mesh[row + 1][column + 1], mesh[row][column + 1]);
    }
    source->UnlockBits(&sourceData);
    result->UnlockBits(&resultData);
    return result;
}

Bitmap* WarpedTouchHead() {
    Bitmap* head = touchHeadImage->Frame();
    if (!head) return nullptr;
    const PointF crop = touchHeadImage->FrameOffset();
    return WarpedHead(touchBackImage->GetWidth(), touchBackImage->GetHeight(), *head,
        PointF(touchHeadOffset.X + crop.X, touchHeadOffset.Y + crop.Y),
        TouchWarpCenterSource(), EffectiveCheekWarpPull(), touchWarpBaseRadius, false);
}

Bitmap* WarpedTouchEndHead() {
    const float scale = TouchReleasePullScale(activeFrame);
    Bitmap* head = touchEndHeadImage->Frame();
    if (!head) return nullptr;
    const PointF crop = touchEndHeadImage->FrameOffset();
    return WarpedHead(touchEndBackImage->GetWidth(), touchEndBackImage->GetHeight(), *head,
        PointF(touchEndHeadOffset.X + crop.X, touchEndHeadOffset.Y + crop.Y),
        touchReleaseWarpCenter, PointF(touchReleasePullSource.X * scale,
            touchReleasePullSource.Y * scale), touchWarpBaseRadius, false);
}

#include "clock-pat-warp.h"

void DrawArtwork(Graphics& g, const SYSTEMTIME& now, bool drawDial, bool drawTicks, bool drawNumbers) {
    const bool endingTouch = touchReleaseActive && activeFrame < TouchBounceFrameCount;
    AnimationImage* primary = cheekDragging || endingTouch ? nullptr : CurrentGif();
    for (AnimationImage* image : ArtworkAnimations()) {
        const bool touchLayer = cheekDragging && (image == touchBackImage.get() ||
            image == touchHeadImage.get() || image == touchFrontImage.get());
        const bool endLayer = endingTouch && (image == touchEndBackImage.get() ||
            image == touchEndHeadImage.get() || image == touchEndFrontImage.get());
        image->SetActive(image == primary || touchLayer || endLayer);
    }
    auto& dial = *clockDial;
    if (drawDial) g.FillPath(&dial.background, dial.clip.get());
    g.SetInterpolationMode(InterpolationModeBilinear);
    GraphicsState clipState = g.Save();
    g.SetClip(dial.clip.get(), CombineModeReplace);
    if (cheekDragging || (touchReleaseActive && activeFrame < TouchBounceFrameCount)) {
        const PointF position = GifPosition();
        auto head = cheekDragging ? WarpedTouchHead() : WarpedTouchEndHead();
        auto drawLayer = [&](Bitmap& image, PointF offset) {
            g.DrawImage(&image, RectF(position.X + offset.X * CharacterScale, position.Y + offset.Y * CharacterScale,
                image.GetWidth() * CharacterScale, image.GetHeight() * CharacterScale),
                0, 0, static_cast<float>(image.GetWidth()), static_cast<float>(image.GetHeight()), UnitPixel);
        };
        AnimationImage& back = cheekDragging ? *touchBackImage : *touchEndBackImage;
        AnimationImage& front = cheekDragging ? *touchFrontImage : *touchEndFrontImage;
        const PointF frontOffset = cheekDragging ? touchFrontOffset : touchEndFrontOffset;
        Bitmap* composite = cheekWarpBuffers.ComposeLayers(back, head, front, frontOffset);
        if (composite) {
            drawLayer(*composite, cheekWarpBuffers.compositeOffset);
        } else {
            if (Bitmap* image = back.Frame()) drawLayer(*image, back.FrameOffset());
            if (head) drawLayer(*head, PointF());
            const PointF crop = front.FrameOffset();
            if (Bitmap* image = front.Frame()) drawLayer(*image, PointF(frontOffset.X + crop.X, frontOffset.Y + crop.Y));
        }
    } else {
        AnimationImage* animation = CurrentGif();
        const PointF position = AnimationPosition(*animation);
        Bitmap* image = animation->Frame();
        PointF crop = animation->FrameOffset();
        if (patActive) {
            Bitmap* warped = WarpedPatImage();
            if (warped) {
                image = warped;
                const float padding = static_cast<float>(PatWarpPadding);
                crop = PointF(-padding, -padding);
            }
        }
        if (image) g.DrawImage(image, RectF(position.X + crop.X * CharacterScale, position.Y + crop.Y * CharacterScale,
            image->GetWidth() * CharacterScale, image->GetHeight() * CharacterScale),
            0, 0, static_cast<float>(image->GetWidth()), static_cast<float>(image->GetHeight()), UnitPixel);
    }
    g.Restore(clipState);
    for (int i = 0; i < 60; ++i) {
        if (ShouldDrawTick(i, drawTicks, drawNumbers)) {
            const bool major = i % 5 == 0;
            g.DrawLine(major ? &dial.majorOutline : &dial.minorOutline, dial.tickStart[i], dial.tickEnd[i]);
            g.DrawLine(major ? &dial.majorTick : &dial.minorTick, dial.tickStart[i], dial.tickEnd[i]);
        }
    }
    if (drawNumbers) {
        for (auto& number : dial.numbers) {
            g.DrawPath(&dial.numberOutline, number.get());
            g.FillPath(&dial.ink, number.get());
        }
    }
    if (HandsVisible()) {
        const ULONGLONG tick = GetTickCount64();
        float turns[] = { HourTurn(now.wHour, now.wMinute, now.wSecond), MinuteTurn(now.wMinute, now.wSecond) };
        if (alarmActive && !alarmPanicActive) {
            const float progress = (tick - animationStart) / static_cast<float>(AlarmHandGatherMs);
            turns[0] = AnimatedHandTurn(alarmHandStartTurns[0], AlarmHandTurn, progress);
            turns[1] = AnimatedHandTurn(alarmHandStartTurns[1], AlarmHandTurn, progress);
        } else if (handReturnActive) {
            const ULONGLONG elapsed = tick - handReturnStart;
            if (elapsed >= HandReturnMs) {
                handReturnActive = false;
            } else {
                const float progress = elapsed / static_cast<float>(HandReturnMs);
                turns[0] = AnimatedHandTurn(handReturnStartTurns[0], turns[0], progress);
                turns[1] = AnimatedHandTurn(handReturnStartTurns[1], turns[1], progress);
            }
        }
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        for (int i = 1; i >= 0; --i) {
            auto& hand = clockHands[i];
            GraphicsState state = g.Save();
            Matrix transform;
            const bool idleHandCenter = (CurrentGif() == gifImage.get() || patReleaseActive) &&
                activeFrame < frameCenters.size();
            const bool transitionHandCenter = CurrentGif() == transitionImage.get() &&
                activeFrame < transitionSourceFrameEnds.size();
            Matrix fixedTipTransform;
            HandTransform(fixedTipTransform, hand, turns[i], clockCenter);
            PointF fixedTip = hand.tip;
            fixedTipTransform.TransformPoints(&fixedTip, 1);
            const float lengthScale = HandLengthScale(i, tick);
            if (lengthScale != 1.0f) {
                fixedTip.X = clockCenter.X + (fixedTip.X - clockCenter.X) * lengthScale;
                fixedTip.Y = clockCenter.Y + (fixedTip.Y - clockCenter.Y) * lengthScale;
            }
            const PointF movingCenter = patActive ? PointF(clockCenter.X + patPull.X, clockCenter.Y + patPull.Y) :
                idleHandCenter ? IdleHandCenter(activeFrame) :
                transitionHandCenter ? TransitionHandCenter(activeFrame) : clockCenter;
            StretchedHandTransform(transform, hand, movingCenter, fixedTip);
            g.MultiplyTransform(&transform);
            g.DrawImage(hand.image.get(), RectF(0, 0, static_cast<float>(hand.image->GetWidth()),
                static_cast<float>(hand.image->GetHeight())), 0, 0, static_cast<float>(hand.image->GetWidth()),
                static_cast<float>(hand.image->GetHeight()), UnitPixel);
            g.Restore(state);
        }
    }
}
