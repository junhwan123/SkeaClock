#pragma once

#include <random>
#include "animation.h"
#include "../graphics/clock-shape.h"
namespace CoffeEffects {
void PlayCoffee1Idle();
void PlayCompletionIfReady();
void Reset();
}

namespace Coffe {
constexpr ULONGLONG CupFadeMs = 300;
constexpr UINT BasicIdleEntryFrame = 9;
constexpr UINT Coffee1IdleSoundFrame = 36 - 1;
constexpr float CheekWarpRadius = 23.0f * SourceImageScale;
const RectF CheekTouchBounds(190.0f, 196.0f, 44.0f, 44.0f);
const RectF HeadTouchBounds(156.6f, 76.0f, 196.8f, 96.0f);

enum Clip {
    Idle, Coffee1Start, Coffee1Idle, Coffee2Start, Coffee2Idle, Coffee3Start, Coffee3Idle, CoffeeEnd,
    Smash, PatStart, PatIdle, PatEnd, TouchStart, TouchIdle, TouchChange, TouchEnd,
    BlankStart, BlankIdle, BlankEnd, ClipCount
};
struct ClipSpec {
    const wchar_t* section;
    Clip next;
    UINT entryFrame;
};
constexpr ClipSpec ClipSpecs[ClipCount] = {
    { L"idle", Idle, 6 },
    { L"coffee1_start", Coffee1Idle, 0 },
    { L"coffee1_idle", Coffee1Idle, 6 },
    { L"coffee2_start", Coffee2Idle, 0 },
    { L"coffee2_idle", Coffee2Idle, 15 },
    { L"coffee3_start", Coffee3Idle, 0 },
    { L"coffee3_idle", Coffee3Idle, 6 },
    { L"coffee_end", Idle, 0 },
    { L"smash", Idle, 0 },
    { L"pat_start", PatIdle, 0 },
    { L"pat_idle", PatIdle, BasicIdleEntryFrame },
    { L"pat_end", Idle, 0 },
    { L"touch_start", TouchIdle, 0 },
    { L"touch_idle", TouchIdle, BasicIdleEntryFrame },
    { L"touch_change", TouchEnd, 0 },
    { L"touch_end", Idle, 0 },
    { L"blank_start", BlankIdle, 0 },
    { L"blank_idle", BlankIdle, BasicIdleEntryFrame },
    { L"blank_end", Idle, 0 },
};

std::unique_ptr<AnimationImage> animations[ClipCount][3];
std::unique_ptr<AnimationImage> smoke, catCupAnimation;
std::vector<PointF> touchWarpCenters[3];
Clip clip = Idle;
AnimationTiming timing, cupTiming;
ULONGLONG cupClickedAt = 0;
PointF anchor;
float scale = 1;
bool useCatCup = false;

float ImageScale() { return scale / SourceImageScale; }

PointF CharacterOrigin() {
    const float imageScale = ImageScale();
    return PointF(clockCenter.X - anchor.X * imageScale, clockCenter.Y - anchor.Y * imageScale);
}

RectF PlaceBounds(const RectF& bounds) {
    const PointF origin = CharacterOrigin();
    return RectF(origin.X + bounds.X * scale, origin.Y + bounds.Y * scale,
        bounds.Width * scale, bounds.Height * scale);
}

PointF BoundsCenter(const RectF& bounds) {
    return PointF(bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2);
}

bool Layered(Clip value) { return value == TouchStart || value == TouchIdle || value == TouchChange; }

AnimationImage& CurrentAnimation() { return *animations[clip][0]; }

bool CupIsFading(ULONGLONG now) {
    return cupClickedAt && now - cupClickedAt < CupFadeMs;
}

AnimationTransition<Clip> NextAnimation() {
    if (clip == PatStart && !patActive) return { PatEnd };
    const Clip next = ClipSpecs[clip].next;
    if (next == clip) return { clip };
    const UINT first = clip == Smash || clip == PatEnd || clip == TouchEnd || clip == BlankEnd ?
        BasicIdleEntryFrame : ClipSpecs[next].entryFrame;
    return { next, first };
}

bool Select(UINT frame, ULONGLONG now, bool immediate = false, bool initial = false) {
    AnimationPrefetchPass pass;
    const auto next = NextAnimation();
    const bool startsCoffeeIdleSound = clip == Coffee1Start &&
        CurrentAnimation().frame < Coffee1IdleSoundFrame && frame >= Coffee1IdleSoundFrame;
    bool ready = SelectLayerFrames(animations[clip], frame, immediate);
    if (ready && startsCoffeeIdleSound) CoffeEffects::PlayCoffee1Idle();
    PrepareFollowingLayers(animations[clip], animations[next.clip], next.frame);
    if (next.clip != clip) PrepareLayerFrames(animations[next.clip], next.frame);
    const auto prepare = [&](Clip value) {
        if (!PrepareLayerFrames(animations[value], 0, initial) && initial) ready = false;
    };
    switch (clip) {
    case Coffee1Start: case Coffee1Idle: prepare(Coffee2Start); break;
    case Coffee2Start: case Coffee2Idle: prepare(Coffee3Start); break;
    case Coffee3Start: case Coffee3Idle: prepare(CoffeeEnd); break;
    default:
        for (Clip value : { Idle, Smash, PatStart, TouchStart, Coffee1Start }) prepare(value);
        break;
    }
    prepare(BlankStart);
    if (blankPlaybackActive) prepare(BlankEnd);
    if (patActive) prepare(PatEnd);
    if (cheekDragging) prepare(TouchChange);
    if (clip == Coffee3Idle || CupIsFading(now)) {
        const ULONGLONG position = cupTiming.Position(now);
        for (auto* image : { smoke.get(), useCatCup ? catCupAnimation.get() : nullptr }) {
            if (!image) continue;
            const UINT index = static_cast<UINT>(position % image->GetFrameCount());
            image->SelectFrame(index, immediate && clip == Coffee3Idle);
            image->PrepareNext(image, 0);
        }
    } else if (clip == Coffee2Idle || clip == Coffee3Start) {
        if (smoke) smoke->Prepare(0);
        if (catCupAnimation) catCupAnimation->Prepare(0);
    }
    return ready;
}

bool PlayClip(Clip next, ULONGLONG now, UINT first = 0, bool initial = false) {
    clip = next;
    if (clip == Coffee3Idle) {
        static std::mt19937 random(std::random_device{}());
        useCatCup = catCupAnimation && std::bernoulli_distribution(0.20)(random);
        cupTiming.Begin(0, now * AnimationFps);
    }
    timing.Begin(first, now * AnimationFps);
    return Select(first, now, true, initial);
}

void BeginSmash() {
    ClearInteractionState();
    PlayClip(Smash, GetTickCount64());
}

void BeginPat() {
    ClearInteractionState();
    patActive = true;
    PlayClip(PatStart, GetTickCount64());
}

void EndPat() {
    ClearInteractionState();
    if (clip != PatStart) PlayClip(PatEnd, GetTickCount64());
}

void BeginTouch() {
    ClearInteractionState();
    cheekDragging = true;
    PlayClip(TouchStart, GetTickCount64());
}

void EndTouch() {
    cheekPull = PointF();
    cheekDragging = false;
    touchReleaseActive = true;
    PlayClip(TouchChange, GetTickCount64());
}

bool IsMakingCoffee(ULONGLONG now) {
    return (clip >= Coffee1Start && clip <= CoffeeEnd) || CupIsFading(now);
}

bool LoadAnimation(const std::wstring& directory) {
    const AnimationLayout config{ directory + L"locate data\\coffe-layout.ini" };
    scale = config.Number(L"layout", L"scale");
    anchor = PointF(config.Number(L"layout", L"x") + .5f, config.Number(L"layout", L"y") + .5f);
    if (!std::isfinite(scale) || scale <= 0) return false;
    const wchar_t* suffixes[] = { L"_back", L"_head", L"_front" };
    for (int index = 0; index < ClipCount; ++index) {
        const bool layered = Layered(static_cast<Clip>(index));
        const auto& spec = ClipSpecs[index];
        for (int layer = 0; layer < (layered ? 3 : 1); ++layer) {
            const std::wstring section = std::wstring(spec.section) + (layered ? suffixes[layer] : L"");
            auto& image = animations[index][layer];
            image = std::make_unique<AnimationImage>();
            if (!image->Load(directory, config.path, section.c_str()) ||
                image->firstFrame != 0 || image->storedFrames != image->GetFrameCount() ||
                !PointInImage(anchor, image->GetWidth(), image->GetHeight()) ||
                spec.entryFrame >= image->GetFrameCount() ||
                (index == Idle && BasicIdleEntryFrame >= image->GetFrameCount()))
                return false;
            const auto& base = *animations[index][0];
            if (layer && (image->GetWidth() != base.GetWidth() || image->GetHeight() != base.GetHeight() ||
                image->frameEnds != base.frameEnds)) return false;
            if (layer == 1 && !config.FrameCenters(section.c_str(), *image, touchWarpCenters[index - TouchStart]))
                return false;
        }
    }
    smoke = std::make_unique<AnimationImage>();
    if (!smoke->Load(directory, config.path, L"smoke") || smoke->firstFrame != 0 ||
        smoke->storedFrames != smoke->GetFrameCount()) smoke.reset();
    catCupAnimation = std::make_unique<AnimationImage>();
    if (!catCupAnimation->Load(directory, config.path, L"cat_cup") || catCupAnimation->firstFrame != 0 ||
        catCupAnimation->storedFrames != catCupAnimation->GetFrameCount()) catCupAnimation.reset();
    return true;
}

void ReleaseAnimation() {
    clip = Idle;
    cupClickedAt = 0;
    useCatCup = false;
    for (auto& layers : animations) for (auto& image : layers) image.reset();
    smoke.reset();
    catCupAnimation.reset();
    for (auto& centers : touchWarpCenters) centers.clear();
}

UINT Advance(ULONGLONG now) {
    auto& image = CurrentAnimation();
    const UINT count = image.GetFrameCount();
    const ULONGLONG position = timing.Position(now);
    const auto next = NextAnimation();
    if (clip == PatStart && position >= count) {
        PlayClip(next.clip, timing.End(count) / AnimationFps, next.frame);
        return timing.Delay(now);
    }
    const bool loop = next.clip == clip;
    const UINT frame = static_cast<UINT>(loop ? position % count : std::min(position, static_cast<ULONGLONG>(count - 1)));
    if (!loop && position >= count && image.frame == frame) {
        if (clip == TouchChange) {
            touchReleaseActive = false;
            cheekWarpBuffers.Release();
        }
        PlayClip(next.clip, now, next.frame);
    } else if (!Select(frame, now)) return 1;
    if (clip != Coffee3Start) CoffeEffects::PlayCompletionIfReady();
    return timing.Delay(now);
}

void DrawCharacter(Graphics& graphics) {
    auto& animation = CurrentAnimation();
    const PointF origin = CharacterOrigin();
    const float imageScale = ImageScale();
    if (Layered(clip)) {
        auto& head = *animations[clip][1];
        auto& front = *animations[clip][2];
        const PointF center = touchWarpCenters[clip - TouchStart][head.frame];
        PointF pull = ClampInwardCheekPull(PointF(cheekPull.X / imageScale, cheekPull.Y / imageScale), CheekWarpRadius);
        if (touchReleaseActive) {
            const float recovery = TouchReleasePullScale(head.frame);
            pull = PointF(touchReleasePullSource.X * recovery, touchReleasePullSource.Y * recovery);
        }
        DrawWarpedLayers(graphics, animation, head, front, origin, imageScale, center, pull, CheekWarpRadius);
        return;
    }
    Bitmap* image = animation.Frame();
    const PointF offset = animation.FrameOffset();
    if (image && patActive) {
        const PointF center = BoundsCenter(HeadTouchBounds);
        const PointF pull(patPull.X / imageScale, patPull.Y / imageScale);
        if (auto* warped = WarpedHead(animation.GetWidth(), animation.GetHeight(), animation, offset,
            PointF(center.X * SourceImageScale, center.Y * SourceImageScale), pull, PatWarpRadius / imageScale, true)) {
            DrawAnimationFrame(graphics, warped, origin, PointF(), imageScale);
            return;
        }
    }
    DrawAnimationFrame(graphics, image, origin, offset, imageScale);
}
}
