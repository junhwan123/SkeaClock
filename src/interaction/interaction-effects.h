#pragma once

#include <random>

constexpr UINT SmashStarDurationMs = 650;
constexpr int SmashStarCount = 6;
bool smashStarActive = false;
PointF smashStarOrigin;
ULONGLONG smashStarStart = 0;
float smashStarHorizontalTravel[SmashStarCount] = {};
float smashStarUpwardVelocity[SmashStarCount] = {};
float smashStarGravity[SmashStarCount] = {};
float smashStarSizes[SmashStarCount] = {};
float smashStarDelays[SmashStarCount] = {};
float smashStarRotations[SmashStarCount] = {};

void BeginSmashStarEffect(PointF origin) {
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_real_distribution<float> variation(-1.0f, 1.0f);
    static std::uniform_real_distribution<float> apexHeight(10.0f, 40.0f);
    static std::uniform_real_distribution<float> fallDistance(25.0f, 50.0f);
    constexpr float baseHorizontal[SmashStarCount] = { -63, -39, -17, 17, 39, 63 };
    smashStarActive = true;
    smashStarOrigin = origin;
    smashStarStart = GetTickCount64();
    for (int i = 0; i < SmashStarCount; ++i) {
        smashStarHorizontalTravel[i] = baseHorizontal[i] + variation(generator) * (i == 0 || i == 5 ? 5 : 9);
        const float height = apexHeight(generator);
        const float landing = fallDistance(generator);
        const float upwardSpeed = 2 * height + 2 * std::sqrt(height * (height + landing));
        smashStarUpwardVelocity[i] = -upwardSpeed;
        smashStarGravity[i] = upwardSpeed + landing;
        smashStarSizes[i] = 14.25f + variation(generator) * 3.0f;
        smashStarDelays[i] = (variation(generator) + 1.0f) * .015f;
        smashStarRotations[i] = variation(generator) * Pi;
    }
}

bool AdvanceSmashStarEffect(ULONGLONG now) {
    if (!smashStarActive) return false;
    if (now - smashStarStart >= SmashStarDurationMs) smashStarActive = false;
    return true;
}

void DrawSmashStar(Graphics& graphics, PointF center, float radius, float rotation, BYTE alpha) {
    PointF points[10];
    for (int i = 0; i < 10; ++i) {
        const float angle = rotation - Pi / 2 + i * Pi / 5;
        const float pointRadius = i % 2 == 0 ? radius : radius * .42f;
        points[i] = PointF(center.X + std::cos(angle) * pointRadius,
            center.Y + std::sin(angle) * pointRadius);
    }
    SolidBrush fill(Color(alpha, 255, 221, 35));
    graphics.FillPolygon(&fill, points, 10);
}

void DrawSmashStarEffect(Graphics& graphics) {
    if (!smashStarActive) return;
    const ULONGLONG elapsed = GetTickCount64() - smashStarStart;
    if (elapsed >= SmashStarDurationMs) return;

    const float progress = elapsed / static_cast<float>(SmashStarDurationMs);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    for (int i = 0; i < SmashStarCount; ++i) {
        const float delay = smashStarDelays[i];
        if (progress <= delay) continue;
        const float local = std::clamp((progress - delay) / (1.0f - delay), 0.0f, 1.0f);
        const float appear = std::min(1.0f, local * 7.0f);
        const float radius = smashStarSizes[i] * appear * (1.0f - local * .45f);
        const PointF center(smashStarOrigin.X + smashStarHorizontalTravel[i] * local,
            smashStarOrigin.Y + smashStarUpwardVelocity[i] * local + smashStarGravity[i] * local * local);
        const BYTE alpha = static_cast<BYTE>(255 * (1.0f - local));
        DrawSmashStar(graphics, center, radius, smashStarRotations[i] + local * 1.8f, alpha);
    }
}

constexpr UINT_PTR PatHeartTimer = 4;
constexpr int PatHeartCount = 3;
constexpr UINT PatHeartSpawnMs = 650;
constexpr UINT PatHeartFadeInMs = 400;
constexpr UINT PatHeartFadeOutMs = PatHeartFadeInMs * 2;
constexpr UINT PatHeartDurationMs = PatHeartFadeInMs + PatHeartFadeOutMs;

struct PatHeart {
    PointF origin;
    float radius = 0;
    ULONGLONG start = 0;
    bool active = false;
};
PatHeart patHearts[PatHeartCount];
ULONGLONG nextPatHeartSpawn = 0;

bool AdvancePatHeartEffect(ULONGLONG now) {
    bool active = false;
    for (auto& heart : patHearts) {
        if (heart.active && now - heart.start >= PatHeartDurationMs) heart.active = false;
        active = active || heart.active;
    }
    if (patActive && now >= nextPatHeartSpawn) {
        static std::mt19937 generator(std::random_device{}());
        static std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        for (auto& heart : patHearts) {
            if (heart.active) continue;
            const float angle = unit(generator) * Pi * 2;
            const float radial = std::sqrt(unit(generator));
            constexpr float radiusY = DialRadius * .3f;
            constexpr float radiusX = radiusY * 1.5f;
            const float dx = std::sin(angle) * radial * radiusX * 1.5f;
            const float dy = -std::cos(angle) * radial * radiusY;
            heart = { PointF(clockCenter.X + dx, clockCenter.Y + dy),
                17.5f * (.75f + .5f * unit(generator)), now, true };
            nextPatHeartSpawn = now + PatHeartSpawnMs;
            active = true;
            break;
        }
    }
    return active || patActive;
}

void BeginPatHeartEffect(HWND window) {
    nextPatHeartSpawn = GetTickCount64();
    AdvancePatHeartEffect(nextPatHeartSpawn);
    SetTimer(window, PatHeartTimer, 33, nullptr);
}

void DrawPatHeartEffect(Graphics& graphics) {
    const ULONGLONG now = GetTickCount64();
    for (const auto& heart : patHearts) {
        if (!heart.active) continue;
        const ULONGLONG elapsed = now - heart.start;
        if (elapsed >= PatHeartDurationMs) continue;
        const float age = static_cast<float>(elapsed);
        const float opacity = elapsed < PatHeartFadeInMs ? age / PatHeartFadeInMs :
            1 - (age - PatHeartFadeInMs) / PatHeartFadeOutMs;
        const float x = heart.origin.X, y = heart.origin.Y - age * .044f;
        const float radius = heart.radius;
        GraphicsPath path;
        path.AddBezier(PointF(x, y + radius * .75f), PointF(x - radius * 1.4f, y - radius * .1f),
            PointF(x - radius * .8f, y - radius * 1.3f), PointF(x, y - radius * .45f));
        path.AddBezier(PointF(x, y - radius * .45f), PointF(x + radius * .8f, y - radius * 1.3f),
            PointF(x + radius * 1.4f, y - radius * .1f), PointF(x, y + radius * .75f));
        path.CloseFigure();
        SolidBrush fill(Color(static_cast<BYTE>(255 * opacity), 255, 95, 145));
        graphics.FillPath(&fill, &path);
    }
}
