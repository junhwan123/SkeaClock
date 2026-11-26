#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <cmath>
#include <climits>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

constexpr wchar_t ClassName[] = L"SkeaClockWindow";
constexpr wchar_t InstanceMutexName[] = L"Local\\SkeaClock.SingleInstance";

int lastStep = -1;
std::wstring settingsPath;

#include "src/app/error.h"
#include "src/interaction/interaction.h"
#include "src/menu/menu.h"
#include "src/app/preload.h"

void UpdateClock(HWND window) {
    if (IsCoffeCharacter()) return;
    SYSTEMTIME now;
    GetLocalTime(&now);
    UpdateAlarm(window, now);
    int step = now.wHour * 3600 + now.wMinute * 60 + now.wSecond;
    if (step == lastStep) return;
    lastStep = step;
    RenderClock(window);
}

void HandleMusicPlaybackResult(HWND window, HRESULT result, AppError failure, std::wstring_view failedPath) {
    const bool spinning = Music::PlaybackRequested();
    if (Coffe::recordSpinning != spinning || result != S_OK) {
        Coffe::SetRecordSpinning(spinning);
        if (IsCoffeCharacter()) RenderClock(window);
    }
    if (FAILED(result)) ShowAppError(window, failure, failedPath);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == AlarmEditorClosedMessage) {
        if (!alarmEditorWindow && !layoutEditing) StopBlankPlayback(window);
        return 0;
    }
    if (message == ExceptionMenuReadyMessage) {
        ApplyExceptionMenuResult(window);
        return 0;
    }
    if (message == WM_INITMENUPOPUP) {
        OpenExceptionMenu(window, reinterpret_cast<HMENU>(wParam));
        return 0;
    }
    if (message == ForegroundChangedMessage) {
        HWND foreground = reinterpret_cast<HWND>(wParam);
        const ForegroundKind kind = ClassifyForeground(window, foreground);
        if (kind != ForegroundKind::None) ApplyForegroundTransition(window, foreground);
        return 0;
    }
    if (message == TaskSwitchChangedMessage) {
        if (wParam == FALSE) {
            HWND foreground = reinterpret_cast<HWND>(lParam);
            if (!foreground || !IsWindow(foreground)) foreground = GetForegroundWindow();
            if (ClassifyForeground(window, foreground) != ForegroundKind::None)
                ApplyForegroundTransition(window, foreground);
        }
        return 0;
    }
    switch (message) {
    case WM_CREATE:
        SetTimer(window, 1, 100, nullptr);
        AnimateBackground(window);
        return 0;
    case Music::EndedMessage: {
        const HRESULT result = Music::RepeatPlayback(window, wParam);
        HandleMusicPlaybackResult(window, result, AppError::RepeatMusicPlayback, Music::selectedFile);
        return 0;
    }
    case WM_TIMER: {
        if (wParam == AnimationTimer) {
            SampleDragPointer(window);
            AnimateBackground(window);
            return 0;
        }
        if (wParam == PatHeartTimer) {
            const bool active = AdvancePatHeartEffect(GetTickCount64());
            if (!IsCoffeCharacter()) RenderClock(window);
            if (!active) KillTimer(window, PatHeartTimer);
            return 0;
        }
        if (wParam == HeadHoldTimer) {
            HoldClockHead(window);
            return 0;
        }
        RecoverTopmostState(window);
        UpdateClock(window);
        CheckAlarmSoundValidation(window);
        ReleaseFinishedTimerAlarmSound();
        std::wstring failedPath;
        if (const auto error = Music::CheckSelection(failedPath)) {
            HandleMusicPlaybackResult(window, E_FAIL, *error, failedPath);
        } else {
            const HRESULT playback = Music::CheckPlayback(window);
            HandleMusicPlaybackResult(window, playback, AppError::CheckMusicPlayback, Music::selectedFile);
        }
        CheckUpdateResult(window);
        return 0;
    }
    case WM_TIMECHANGE:
        lastStep = -1;
        lastAlarmMinuteKey = ULLONG_MAX;
        UpdateClock(window);
        return 0;
    case WM_NCHITTEST:
        if (!layoutEditing && (clockTopInset || clockBottomInset)) {
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT clock;
            ScreenToClient(window, &point);
            if (GetClockClientRect(window, clock)) {
                if (Kommy::TimeFieldAt(clock, point) != Kommy::TimeField::None) return HTCLIENT;
                const auto panel = Kommy::GetTimePanelSize(clock.right - clock.left);
                const float frameInset = panel.overlap - panel.inset;
                const float top = clock.top + (clockTopInset ? frameInset : 0.0f);
                const float bottom = clock.bottom - (clockBottomInset ? frameInset : 0.0f);
                if (point.y < top || point.y >= bottom) return HTTRANSPARENT;
            }
        }
        break;

    case WM_LBUTTONDOWN:
        PressClock(window, lParam);
        return 0;
    case WM_MOUSEMOVE:
        MoveClockPointer(window, wParam, lParam);
        return 0;
    case WM_LBUTTONUP:
        ReleaseClockPointer(window, lParam);
        return 0;
    case WM_CAPTURECHANGED:
    case WM_CANCELMODE:
        CancelClockInput(window, message == WM_CANCELMODE);
        return 0;

    case WM_SETCURSOR:
        if (layoutEditing && LOWORD(lParam) == HTCLIENT) {
            POINT cursor;
            GetCursorPos(&cursor);
            ScreenToClient(window, &cursor);
            SetCursor(LayoutCursor(LayoutHitAt(window,
                static_cast<float>(cursor.x), static_cast<float>(cursor.y))));
            return TRUE;
        }
        break;
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DPICHANGED:
        Kommy::CloseTimeEditor();
        PositionClock(window);
        RenderClock(window);
        return 0;
    case WM_CONTEXTMENU: {
        Kommy::CloseTimeEditor();
        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (point.x == -1 && point.y == -1) GetCursorPos(&point);
        ShowMenu(window, point);
        return 0;
    }

    case WM_COMMAND:
        HandleMenuCommand(window, LOWORD(wParam));
        return 0;

    case WM_CLOSE:
        SaveCurrentLayout(window);
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, 1);
        KillTimer(window, AnimationTimer);
        KillTimer(window, HeadHoldTimer);
        KillTimer(window, PatHeartTimer);
        StopForegroundMonitor();
        CancelAlarmSoundValidation();
        updateCheck.reset();
        Music::Shutdown();
        ShutdownSoundSystem();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, FALSE, InstanceMutexName);
    if (!mutex) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(ClassName, nullptr);
        if (existing) ShowWindow(existing, SW_SHOWNOACTIVATE);
        CloseHandle(mutex);
        return 0;
    }
    ULONG_PTR token = 0;
    if (!PreloadApplication(token)) {
        CloseHandle(mutex);
        return 1;
    }
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = AppIcon();
    windowClass.lpszClassName = ClassName;
    RegisterClassW(&windowClass);
    HWND window = CreateWindowExW(ClockExtendedStyle(),
        ClassName, L"Skea clock", WS_POPUP, 0, 0, 240, 240, nullptr, nullptr, instance, nullptr);
    int result = 1;
    if (window) {
        PositionClock(window);
        SYSTEMTIME startupTime;
        GetLocalTime(&startupTime);
        InitializeAlarmMinuteTracking(startupTime);
        PlayIdleSound();
        UpdateClock(window);
        ShowWindow(window, SW_SHOWNOACTIVATE);
        StartForegroundMonitor(window);
        ApplyTopmostState(window, true);
        Music::ValidateSavedSong(window);
        ValidateSavedAlarmSound(window);
        BeginUpdateCheck();
        MSG message;
        int status;
        while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        result = status == -1 ? 1 : static_cast<int>(message.wParam);
    } else {
        ShowAppError(nullptr, AppError::CreateClockWindow);
    }
    renderBuffer.Release();
    ReleaseArtwork();
    GdiplusShutdown(token);
    CloseHandle(mutex);
    return result;
}
