#pragma once

#include "../graphics/warp.h"

constexpr UINT PatHoldMs = 200;
bool headPressPending = false;
bool patActive = false, cheekDragging = false, touchReleaseActive = false;
bool alarmActive = false, alarmPanicActive = false, blankPlaybackActive = false;
PointF cheekPull, patPull, touchReleasePullSource;

void ClearInteractionState() {
    cheekWarpBuffers.Release();
    patActive = cheekDragging = touchReleaseActive = false;
    alarmActive = alarmPanicActive = false;
    cheekPull = patPull = touchReleasePullSource = PointF();
}

struct HitEllipse {
    PointF center;
    float radiusX, radiusY;

    bool Contains(PointF point) const {
        return std::hypot((point.X - center.X) / radiusX, (point.Y - center.Y) / radiusY) <= 1.0f;
    }
};

#include "../window/window-render.h"

void StartBlankPlayback(HWND window) {
    StopAlarmSound();
    BeginBlankPlayback();
    PlaySoundFile(IsMuteCharacter() || IsKommyCharacter() || IsCoffeCharacter() ? L"blank.wav" : L"blank1.wav");
    RenderClock(window);
    AnimateBackground(window);
}

void StopBlankPlayback(HWND window) {
    EndBlankPlayback();
    RenderClock(window);
    AnimateBackground(window);
}

void StartReaction(HWND window) {
    StopAlarmSound();
    BeginReaction();
    PlaySoundFile(L"smash.wav");
    RenderClock(window);
    AnimateBackground(window);
}

void StartCheekPull(HWND window) {
    StopAlarmSound();
    BeginCheekPull();
    PlaySoundFile(L"touch_start.wav", EffectSoundCategory::Start);
    SetCapture(window);
    RenderClock(window);
    AnimateBackground(window);
}

void FinishCheekPull(HWND window) {
    if (!cheekDragging) return;
    EndCheekPull();
    PlayInteractionEndSound(L"touch");
    RenderClock(window);
    AnimateBackground(window);
}

void FinishLayoutDrag(HWND window, bool releaseCapture) {
    if (layoutDrag == LayoutHandle::None) return;
    layoutDrag = LayoutHandle::None;
    if (releaseCapture && GetCapture() == window) ReleaseCapture();
    SaveCurrentLayout(window);
    RenderClock(window);
}

PointF ClickPoint(HWND window, int x, int y) {
    RECT rect;
    if (!GetClockClientRect(window, rect)) return PointF(-1, -1);
    const int height = rect.bottom - rect.top;
    if (rect.right <= 0 || height <= 0) return PointF(-1, -1);
    return PointF(x * artworkWidth / static_cast<float>(rect.right),
        (y - rect.top) * artworkHeight / static_cast<float>(height));
}

PointF ClickPoint(HWND window, LPARAM position) {
    return ClickPoint(window, GET_X_LPARAM(position), GET_Y_LPARAM(position));
}

PointF ClampedCheekPull(PointF mouse) {
    const PointF center = CheekTouchCenter();
    float dx = mouse.X - center.X, dy = mouse.Y - center.Y;
    const float distance = std::hypot(dx, dy);
    const float limit = CheekTouchRadius() * 1.5f;
    if (distance > limit) {
        dx *= limit / distance;
        dy *= limit / distance;
    }
    return PointF(dx, dy);
}

PointF ClampedPatPull(PointF mouse) {
    const PointF center = HeadTouchArea().center;
    const float dx = mouse.X - center.X, dy = mouse.Y - center.Y;
    const float distance = std::hypot(dx, dy);
    const float radius = IsMuteCharacter() ? Mute::patWarpRadius * Mute::Scale() : PatWarpRadius;
    const float scale = distance > radius ? PatFollowFraction * radius / distance : PatFollowFraction;
    return PointF(dx * scale, dy * scale);
}

constexpr UINT_PTR HeadHoldTimer = 3;
constexpr float PatMoveTolerancePx = 5.0f;
ULONGLONG headPressStart = 0;
PointF headPressPoint;
PointF headPressMouse;
POINT headPressClientPoint = {};

void StartPat(HWND window) {
    KillTimer(window, HeadHoldTimer);
    headPressPending = false;
    StopAlarmSound();
    smashStarActive = false;
    BeginPat();
    BeginPatHeartEffect(window);
    PlaySoundFile(L"pat_start.wav", EffectSoundCategory::Start);
    patPull = ClampedPatPull(headPressMouse);
    RenderClock(window);
    AnimateBackground(window);
}

void UpdateHeadPress(HWND window, LPARAM position) {
    if (!headPressPending && !patActive) return;
    headPressMouse = ClickPoint(window, position);
    if (headPressPending && std::hypot(GET_X_LPARAM(position) - headPressClientPoint.x,
        GET_Y_LPARAM(position) - headPressClientPoint.y) > PatMoveTolerancePx) {
        StartPat(window);
        return;
    }
    if (patActive) patPull = ClampedPatPull(headPressMouse);
}

void FinishHeadPress(HWND window, bool released) {
    KillTimer(window, HeadHoldTimer);
    const bool pending = headPressPending;
    const ULONGLONG elapsed = GetTickCount64() - headPressStart;
    headPressPending = false;
    if (pending && IsKommyCharacter() && !released) {
        Kommy::SetHandVisible(true);
        return;
    }
    if (pending && released) {
        if (elapsed < PatHoldMs) {
            BeginSmashStarEffect(headPressPoint);
            StartReaction(window);
            return;
        }
        StartPat(window);
    }
    if (!patActive) return;
    EndPat();
    PlayInteractionEndSound(L"pat");
    RenderClock(window);
    AnimateBackground(window);
}

void HoldClockHead(HWND window) {
    KillTimer(window, HeadHoldTimer);
    if (headPressPending && GetCapture() == window) {
        StartPat(window);
    }
}

void PressClock(HWND window, LPARAM lParam) {
    const PointF point = ClickPoint(window, lParam);
    if (layoutEditing) {
        const LayoutHandle handle = LayoutHitAt(window,
            static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
        if (handle == LayoutHandle::Close) {
            layoutEditing = false;
            SaveCurrentLayout(window);
            StopBlankPlayback(window);
            PlaySoundFile(L"answer.wav");
        } else if (handle != LayoutHandle::None) {
            BeginLayoutDrag(window, handle);
        }
        return;
    }
    if (IsCoffeCharacter()) {
        if (Coffe::ToggleRecord(window, point) || Coffe::BeginInteraction(window, point)) {
            RenderClock(window);
            return;
        }
        if (Coffe::IsMakingCoffee(GetTickCount64()) || Coffe::IsDragging()) return;
    }
    if (IsKommyCharacter()) {
        RECT clock;
        if (Kommy::showTime && GetClockClientRect(window, clock)) {
            const auto field = Kommy::TimeFieldAt(clock, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
            if (field != Kommy::TimeField::None) {
                Kommy::OpenTimeEditor(window, clock, field);
                return;
            }
        }
        Kommy::CloseTimeEditor();
        if (point.Y < 0 || point.Y >= artworkHeight) return;
        if (Kommy::BeginHandDrag(point)) {
            SetCapture(window);
            return;
        }
    }
    if (HeadTouchArea().Contains(point)) {
        if (IsKommyCharacter()) Kommy::SetHandVisible(false);
        headPressPending = true;
        headPressStart = GetTickCount64();
        headPressPoint = point;
        headPressMouse = point;
        headPressClientPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SetCapture(window);
        if (IsMuteCharacter()) {
            Mute::Advance(GetTickCount64());
            if (Mute::clip == Mute::PatChange) {
                StartPat(window);
                return;
            }
        }
        SetTimer(window, HeadHoldTimer, PatHoldMs, nullptr);
    }
    else {
        const PointF center = CheekTouchCenter();
        if (std::hypot(point.X - center.X, point.Y - center.Y) <= CheekTouchRadius()) StartCheekPull(window);
    }
}

void SampleDragPointer(HWND window) {
    const bool draggingProp = IsCoffeCharacter() && Coffe::IsDragging();
    const bool draggingHand = IsKommyCharacter() && Kommy::draggingHand;
    if ((!draggingProp && !draggingHand) || GetCapture() != window) return;

    static ULONGLONG lastFrame = ULLONG_MAX;
    const ULONGLONG frame = GetTickCount64() * AnimationFps / FrameUnits;
    if (lastFrame == frame) return;
    lastFrame = frame;

    POINT cursor;
    if (!GetCursorPos(&cursor) || !ScreenToClient(window, &cursor)) return;
    const PointF point = ClickPoint(window, cursor.x, cursor.y);
    if (draggingProp) Coffe::MoveDrag(point);
    else Kommy::DragHand(point);
}

void MoveClockPointer(HWND window, WPARAM wParam, LPARAM lParam) {
    if (layoutEditing) {
        if (layoutDrag != LayoutHandle::None) {
            UpdateLayoutDrag(window);
            RenderClock(window);
        } else {
            SetCursor(LayoutCursor(LayoutHitAt(window,
                static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)))));
        }
        return;
    }
    if ((IsCoffeCharacter() && Coffe::IsDragging()) ||
        (IsKommyCharacter() && Kommy::draggingHand)) return;
    if (wParam & MK_LBUTTON) UpdateHeadPress(window, lParam);
    if (cheekDragging) cheekPull = ClampedCheekPull(ClickPoint(window, lParam));
}

void ReleaseClockPointer(HWND window, LPARAM lParam) {
    if (layoutEditing && layoutDrag != LayoutHandle::None) {
        FinishLayoutDrag(window, true);
        return;
    }
    if (IsCoffeCharacter() && Coffe::IsDragging()) {
        if (Coffe::FinishDrag(window, ClickPoint(window, lParam))) RenderClock(window);
        return;
    }
    if (IsKommyCharacter() && Kommy::draggingHand) {
        Kommy::DragHand(ClickPoint(window, lParam));
        Kommy::EndHandDrag();
        if (GetCapture() == window) ReleaseCapture();
        RenderClock(window);
        return;
    }
    if (headPressPending || patActive) {
        UpdateHeadPress(window, lParam);
        FinishHeadPress(window, true);
        if (GetCapture() == window) ReleaseCapture();
        return;
    }
    if (cheekDragging) {
        ReleaseCapture();
        FinishCheekPull(window);
    }
}

void CancelClockInput(HWND window, bool cancelMode) {
    if (IsCoffeCharacter() && Coffe::CancelDrag(window, cancelMode)) {
        RenderClock(window);
        return;
    }
    const bool interactionCaptured = headPressPending || patActive || cheekDragging || Kommy::draggingHand;
    Kommy::EndHandDrag();
    if (layoutDrag != LayoutHandle::None) FinishLayoutDrag(window, false);
    FinishHeadPress(window, false);
    if (cancelMode && interactionCaptured && GetCapture() == window) ReleaseCapture();
    FinishCheekPull(window);
}
