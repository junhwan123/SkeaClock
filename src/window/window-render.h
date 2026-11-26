#pragma once

#include "../clock/function.h"
#include "../interaction/interaction-effects.h"
#include "window-layout.h"

struct ClockRenderBuffer {
    HDC memory = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
    void* pixels = nullptr;
    int width = 0, height = 0;

    void Release() {
        if (previous) SelectObject(memory, previous);
        if (bitmap) DeleteObject(bitmap);
        if (memory) DeleteDC(memory);
        memory = nullptr;
        bitmap = nullptr;
        previous = nullptr;
        pixels = nullptr;
        width = height = 0;
    }

    bool Ensure(int newWidth, int newHeight) {
        if (newWidth <= 0 || newHeight <= 0) return false;
        if (bitmap && width == newWidth && height == newHeight) return true;
        Release();
        memory = CreateCompatibleDC(nullptr);
        if (!memory) return false;
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = newWidth;
        info.bmiHeader.biHeight = -newHeight;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        bitmap = CreateDIBSection(memory, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!bitmap) { Release(); return false; }
        previous = SelectObject(memory, bitmap);
        if (!previous || previous == HGDI_ERROR) {
            previous = nullptr;
            Release();
            return false;
        }
        width = newWidth;
        height = newHeight;
        return true;
    }
};

ClockRenderBuffer renderBuffer;

void RenderClock(HWND window) {
    RECT rect, clock;
    GetClientRect(window, &rect);
    if (!GetClockClientRect(window, clock)) return;
    const int width = rect.right, height = rect.bottom;
    const int clockHeight = clock.bottom - clock.top;
    if (clockHeight <= 0) return;
    if (!renderBuffer.Ensure(width, height)) return;
    {
        Bitmap surface(width, height, width * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(renderBuffer.pixels));
        Graphics g(&surface);
        g.Clear(Color(0, 0, 0, 0));
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.TranslateTransform(0, static_cast<float>(clock.top));
        g.ScaleTransform(width / static_cast<float>(artworkWidth), clockHeight / static_cast<float>(artworkHeight));
        SYSTEMTIME now;
        GetLocalTime(&now);
        DrawArtwork(g, now);
        DrawSmashStarEffect(g);
        DrawPatHeartEffect(g);
        g.ResetTransform();
        Kommy::DrawTimePanel(g, clock);
        g.TranslateTransform(0, static_cast<float>(clock.top));
        DrawLayoutEditor(g, width, clockHeight);
        g.Flush(FlushIntentionSync);
    }
    SIZE dimensions = { width, height };
    POINT origin = {};
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(window, nullptr, nullptr, &dimensions, renderBuffer.memory,
        &origin, 0, &blend, ULW_ALPHA);
}

constexpr UINT_PTR AnimationTimer = 2;

void AnimateBackground(HWND window) {
    KillTimer(window, AnimationTimer);
    const ULONGLONG now = GetTickCount64();
    Kommy::AdvanceTimer(now);
    UINT delay;
    if (IsCoffeCharacter()) {
        delay = Coffe::Advance(now);
    } else if (IsKommyCharacter()) {
        delay = Kommy::Advance(now);
    } else {
        const bool wasPanic = alarmPanicActive;
        delay = IsMuteCharacter() ? Mute::Advance(now) : Skea::Advance(now);
        if (alarmActive && alarmPanicActive && !wasPanic) PlaySoundFile(L"alert.wav");
    }
    AdvanceSmashStarEffect(now);
    RenderClock(window);
    SetTimer(window, AnimationTimer, delay, nullptr);
}
