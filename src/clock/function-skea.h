#pragma once

#include "../animation/animation-skea.h"
#include "../graphics/skea-dial.h"

bool showDial = true, showTicks = true, showNumbers = true;

namespace Skea {
constexpr float HandScale = .9f / SourceImageScale;
constexpr float TimeHandLengthScales[] = { .95f, 1.05f };
constexpr UINT AlarmHandGatherMs = 100;
ClockHand clockHands[2];

float MinuteTurn(int minute, int second) {
    return (minute + second / 60.0f) / 60.0f;
}

float HourTurn(int hour, int minute, int second) {
    return ((hour % 12) + MinuteTurn(minute, second)) / 12.0f;
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
        progress = (tick - playback.start / AnimationFps) / static_cast<float>(AlarmHandGatherMs);
    } else if (handReturnActive) {
        start = handReturnStartLengthScales[index];
        progress = (tick - handReturnStart) / static_cast<float>(handReturnDuration);
    }
    return start + (target - start) * HandAnimationProgress(progress);
}

void BeginAlarm(const SYSTEMTIME& now) {
    const ULONGLONG tick = GetTickCount64();
    for (int i = 0; i < 2; ++i) alarmHandStartLengthScales[i] = HandLengthScale(i, tick);
    ClearInteraction();
    alarmActive = true;
    alarmHandStartTurns[0] = HourTurn(now.wHour, now.wMinute, now.wSecond);
    alarmHandStartTurns[1] = MinuteTurn(now.wMinute, now.wSecond);
    Play(AlertStart, 0, tick * AnimationFps);
}

void DismissAlarmToIdle() {
    const ULONGLONG now = GetTickCount64();
    const float lengths[] = { HandLengthScale(0, now), HandLengthScale(1, now) };
    ClearInteraction();
    Play(Idle, 0, now * AnimationFps);
    StartHandReturn(AlarmHandTurn, now);
    for (int i = 0; i < 2; ++i) handReturnStartLengthScales[i] = lengths[i];
}

bool Load(const std::wstring& directory) {
    const AnimationLayout config{ directory + L"locate data\\skea-layout.ini" };
    if (!clockDial->InitializeMarkings(SkeaDial::Numbers, SkeaDial::Ticks) ||
        !LoadAnimation(directory, config)) return false;
    const wchar_t* roles[] = { L"hour", L"minute" };
    for (int i = 0; i < 2; ++i) {
        auto& hand = clockHands[i];
        hand.image.reset(Bitmap::FromFile((directory + config.Read(roles[i], L"file")).c_str()));
        if (!hand.image || hand.image->GetLastStatus() != Ok) return false;
        hand.center = PointF(config.Number(roles[i], L"centerX") + .5f, config.Number(roles[i], L"centerY") + .5f);
        hand.tip = PointF(config.Number(roles[i], L"tipX") + .5f, config.Number(roles[i], L"tipY") + .5f);
        const UINT width = hand.image->GetWidth(), height = hand.image->GetHeight();
        const float dx = hand.tip.X - hand.center.X, dy = hand.tip.Y - hand.center.Y;
        if (!PointInImage(hand.center, width, height) || !PointInImage(hand.tip, width, height) || (dx == 0 && dy == 0)) return false;
        hand.nativeAngle = std::atan2(dx, -dy) * 180 / Pi;
    }
    ClearInteraction();
    return Play(Idle, 0, GetTickCount64() * AnimationFps, true);
}

void Release() {
    ReleaseAnimation();
    for (auto& hand : clockHands) hand.image.reset();
}

void HandTransform(Matrix& transform, const ClockHand& hand, float turns, PointF center) {
    transform.Translate(center.X, center.Y);
    transform.Rotate(turns * 360 - hand.nativeAngle);
    transform.Scale(HandScale, HandScale);
    transform.Translate(-hand.center.X, -hand.center.Y);
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

void Draw(Graphics& g, const SYSTEMTIME& now, bool drawDial, bool drawTicks, bool drawNumbers) {
    auto& dial = *clockDial;
    g.SetInterpolationMode(InterpolationModeBilinear);
    const GraphicsState clipState = dial.BeginScene(g, drawDial);
    DrawCharacter(g);
    dial.EndScene(g, clipState, drawDial);
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
            const float progress = (tick - playback.start / AnimationFps) / static_cast<float>(AlarmHandGatherMs);
            turns[0] = AnimatedHandTurn(alarmHandStartTurns[0], AlarmHandTurn, progress);
            turns[1] = AnimatedHandTurn(alarmHandStartTurns[1], AlarmHandTurn, progress);
        } else if (handReturnActive) {
            const ULONGLONG elapsed = tick - handReturnStart;
            const UINT duration = handReturnDuration;
            if (elapsed >= duration) {
                handReturnActive = false;
            } else {
                const float progress = elapsed / static_cast<float>(duration);
                turns[0] = AnimatedHandTurn(handReturnStartTurns[0], turns[0], progress);
                turns[1] = AnimatedHandTurn(handReturnStartTurns[1], turns[1], progress);
            }
        }
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        for (int i = 1; i >= 0; --i) {
            auto& hand = clockHands[i];
            GraphicsState state = g.Save();
            Matrix transform;
            Matrix fixedTipTransform;
            HandTransform(fixedTipTransform, hand, turns[i], clockCenter);
            PointF fixedTip = hand.tip;
            fixedTipTransform.TransformPoints(&fixedTip, 1);
            const float lengthScale = HandLengthScale(i, tick);
            if (lengthScale != 1.0f) {
                fixedTip.X = clockCenter.X + (fixedTip.X - clockCenter.X) * lengthScale;
                fixedTip.Y = clockCenter.Y + (fixedTip.Y - clockCenter.Y) * lengthScale;
            }
            const PointF pull = cheekWarpBuffers.appliedPull;
            const PointF movingCenter = patActive ?
                PointF(clockCenter.X + pull.X * scale, clockCenter.Y + pull.Y * scale) : HandCenter(tick);
            StretchedHandTransform(transform, hand, movingCenter, fixedTip);
            g.MultiplyTransform(&transform);
            g.DrawImage(hand.image.get(), RectF(0, 0, static_cast<float>(hand.image->GetWidth()),
                static_cast<float>(hand.image->GetHeight())), 0, 0, static_cast<float>(hand.image->GetWidth()),
                static_cast<float>(hand.image->GetHeight()), UnitPixel);
            g.Restore(state);
        }
    }
}
}
