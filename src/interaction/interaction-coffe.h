#pragma once

#include "../app/error.h"
#include "../clock/function-coffe.h"

namespace CoffeEffects {
ULONGLONG coffee1SoundGeneration = 0;
ULONGLONG coffee3SoundGeneration = 0;

void PlayCoffee1Start() {
    auto& channel = CharacterEffectChannel();
    channel.Play(characterSoundDirectory + L"coffee1 start.wav", effectSoundVolume / 2);
    coffee1SoundGeneration = channel.generation;
}

void PlayCoffee1Idle() {
    auto& channel = CharacterEffectChannel();
    if (!coffee1SoundGeneration || channel.generation != coffee1SoundGeneration) return;
    channel.Play(characterSoundDirectory + L"coffee1 idle1.wav", effectSoundVolume / 2,
        characterSoundDirectory + L"coffee1 idle2.wav");
    coffee1SoundGeneration = channel.generation;
}

void PlayCoffee2Start() {
    coffee1SoundGeneration = 0;
    CharacterEffectChannel().Play(characterSoundDirectory + L"coffee2 start.wav", effectSoundVolume / 2);
}

void PlayCoffee3Start() {
    auto& channel = CharacterEffectChannel();
    channel.Play(characterSoundDirectory + L"coffee3 start.wav", effectSoundVolume / 2);
    coffee3SoundGeneration = channel.generation;
}

void PlayDrinking() {
    CharacterEffectChannel().Play(characterSoundDirectory + L"drinking.wav", effectSoundVolume / 2);
}

void PlayCompletionIfReady() {
    if (!coffee3SoundGeneration) return;
    auto& channel = CharacterEffectChannel();
    if (channel.generation != coffee3SoundGeneration) {
        coffee3SoundGeneration = 0;
    } else if (!(channel.header.dwFlags & WHDR_INQUEUE)) {
        coffee3SoundGeneration = 0;
        channel.Play(characterSoundDirectory + L"coffee end1.wav", effectSoundVolume / 2);
    }
}

void Reset() {
    auto& channel = CharacterEffectChannel();
    if (coffee1SoundGeneration && channel.generation == coffee1SoundGeneration) channel.Stop();
    coffee1SoundGeneration = coffee3SoundGeneration = 0;
}
}

namespace Coffe {
const RectF GiftFaceBounds(181.0f, 85.4f, 152.0f, 156.6f);

bool ToggleRecord(HWND owner, PointF point) {
    if (!showPlayer) return false;
    const float radius = RecordButtonBounds.Width / 2;
    const float dx = point.X - (RecordButtonBounds.X + radius);
    const float dy = point.Y - (RecordButtonBounds.Y + radius);
    if (dx * dx + dy * dy > radius * radius) return false;
    if (!Music::CanPlay()) return true;
    const bool playing = !recordSpinning;
    const HRESULT result = Music::SetPlaying(owner, playing);
    if (SUCCEEDED(result)) SetRecordSpinning(Music::PlaybackRequested());
    else ShowAppError(owner, AppError::ToggleMusicPlayback, Music::selectedFile);
    return true;
}

HitEllipse HeadTouchArea() {
    const RectF bounds = PlaceBounds(HeadTouchBounds);
    return { BoundsCenter(bounds), bounds.Width / 2, bounds.Height / 2 };
}

PointF CheekTouchCenter() { return BoundsCenter(PlaceBounds(CheekTouchBounds)); }
float CheekTouchRadius() { return CheekTouchBounds.Width * scale / 2; }

RectF LowerTouchBounds(const RectF& face) {
    const float width = face.Width / 2;
    const float top = face.GetBottom();
    return RectF(face.X + width / 2, top, width, std::max(0.0f, TableTop() - top));
}

Clip GiftClip(ULONGLONG now) {
    if (!IsMakingCoffee(now)) return Coffee1Start;
    switch (clip) {
    case Coffee1Idle: return Coffee2Start;
    case Coffee2Idle: return Coffee3Start;
    default: return ClipCount;
    }
}

bool IsDragging() { return draggingProp; }

bool BeginInteraction(HWND window, PointF point) {
    const ULONGLONG now = GetTickCount64();
    const auto* prop = VisibleTableProp(now);
    if (IsDragging() || !showTable || !prop || !TablePropBounds(*prop).Contains(point)) return false;
    if (clip == Coffee3Idle) {
        cupClickedAt = now;
        PlayClip(CoffeeEnd, now);
        CoffeEffects::PlayDrinking();
        return true;
    }
    if (GiftClip(now) == ClipCount) return false;
    draggingProp = true;
    dragStart = dragPointer = point;
    SetCapture(window);
    return true;
}

void MoveDrag(PointF point) { dragPointer = point; }

bool CancelDrag(HWND window, bool releaseCapture) {
    if (!IsDragging()) return false;
    draggingProp = false;
    if (releaseCapture && GetCapture() == window) ReleaseCapture();
    return true;
}

bool FinishDrag(HWND window, PointF point) {
    if (!IsDragging()) return false;
    const ULONGLONG now = GetTickCount64();
    const RectF face = PlaceBounds(GiftFaceBounds);
    const bool given = face.Contains(point) || LowerTouchBounds(face).Contains(point);
    const Clip next = GiftClip(now);
    CancelDrag(window, true);
    if (given && next != ClipCount) {
        ClearInteractionState();
        PlayClip(next, now);
        switch (next) {
        case Coffee1Start: CoffeEffects::PlayCoffee1Start(); break;
        case Coffee2Start: CoffeEffects::PlayCoffee2Start(); break;
        case Coffee3Start: CoffeEffects::PlayCoffee3Start(); break;
        default: break;
        }
    }
    return true;
}
}
