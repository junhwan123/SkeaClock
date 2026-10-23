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
#include "clock-resources.h"
#include "clock-localization.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

#ifdef SKEA_TEST_BUILD
constexpr wchar_t ClassName[] = L"SkeaClockTestWindow";
constexpr wchar_t InstanceMutexName[] = L"Local\\SkeaClock.Test.SingleInstance";
#else
constexpr wchar_t ClassName[] = L"SkeaClockWindow";
constexpr wchar_t InstanceMutexName[] = L"Local\\SkeaClock.SingleInstance";
#endif
constexpr int AppIconResource = SKEA_APP_ICON;
constexpr int Background = 101, Ticks = 103, Numbers = 104;
constexpr int AlwaysOnTop = 105, Exit = 106, EditLayout = 107;
constexpr int DisplayFirst = 200, MaxDisplays = 32;
constexpr float Pi = 3.14159265358979323846f;
bool showDial = true, showTicks = true, showNumbers = true, topmost = true;
int lastStep = -1, sizePercent = 100;
unsigned long long lastAlarmMinuteKey = ULLONG_MAX;
unsigned long long activeAlarmMinuteKey = ULLONG_MAX;
std::wstring settingsPath;
std::wstring selectedDisplay, activeDisplay;

struct DisplayInfo {
    MONITORINFOEXW info;
};

HICON AppIcon() {
    return LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(AppIconResource));
}

BOOL CALLBACK CollectDisplay(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto displays = reinterpret_cast<std::vector<DisplayInfo>*>(data);
    DisplayInfo display = { { sizeof(MONITORINFOEXW) } };
    if (GetMonitorInfoW(monitor, &display.info)) displays->push_back(display);
    return TRUE;
}

std::vector<DisplayInfo> GetDisplays() {
    std::vector<DisplayInfo> displays;
    EnumDisplayMonitors(nullptr, nullptr, CollectDisplay, reinterpret_cast<LPARAM>(&displays));
    return displays;
}

const DisplayInfo* ChooseDisplay(const std::vector<DisplayInfo>& displays) {
    for (const auto& display : displays)
        if (!selectedDisplay.empty() && _wcsicmp(display.info.szDevice, selectedDisplay.c_str()) == 0) return &display;
    for (const auto& display : displays)
        if (display.info.dwFlags & MONITORINFOF_PRIMARY) return &display;
    return displays.empty() ? nullptr : &displays.front();
}

std::wstring DisplayLabel(const DisplayInfo& display) {
    std::wstring device = display.info.szDevice;
    const size_t number = device.rfind(L"DISPLAY");
    if (number != std::wstring::npos) device = device.substr(number + 7);
    const RECT& rect = display.info.rcMonitor;
    std::wstring label = UiText().displayPrefix + device + L"  (" +
        std::to_wstring(rect.right - rect.left) + L" × " + std::to_wstring(rect.bottom - rect.top) + L")";
    if (display.info.dwFlags & MONITORINFOF_PRIMARY) label += UiText().displayPrimary;
    return label;
}

float MinuteTurn(int minute, int second) {
    return (minute + second / 60.0f) / 60.0f;
}

float HourTurn(int hour, int minute, int second) {
    return ((hour % 12) + MinuteTurn(minute, second)) / 12.0f;
}

std::wstring FileName(const std::wstring& path) {
    const size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? path : path.substr(separator + 1);
}

#ifdef SKEA_TEST_BUILD
#include "clock-head-debug.h"
#endif
#include "clock-artwork.h"
#include "clock-smash-effect.h"
#include "clock-pat-heart-effect.h"
#include "clock-sound.h"
#include "clock-alarm-validation.h"
#include "clock-window-behavior.h"
#include "clock-menu-icons.h"
#include "clock-exception-menu.h"
#include "clock-language-menu.h"
#include "clock-update-check.h"
#include "clock-startup.h"
#include "clock-layout-editor.h"
#include "alarm-editor.h"
#include "clock-render-buffer.h"

#ifdef SKEA_TEST_BUILD
void DrawHeadPressRange(Graphics& graphics);
#endif

void SaveSettings() {
    WritePrivateProfileStringW(L"clock", L"background", showDial ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"topmost", topmost ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"ticks", showTicks ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"numbers", showNumbers ? L"1" : L"0", settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"markings", nullptr, settingsPath.c_str());
    WritePrivateProfileStringW(L"clock", L"display", selectedDisplay.c_str(), settingsPath.c_str());
    SaveLanguageSettings(settingsPath);
    WritePrivateProfileStringW(L"clock", L"size", nullptr, settingsPath.c_str());
    SaveSoundSettings(settingsPath);
    SaveWindowBehaviorSettings(settingsPath);
    WritePrivateProfileStringW(L"updates", L"enabled", autoCheckUpdates ? L"1" : L"0", settingsPath.c_str());
}

void RenderClock(HWND window) {
    RECT rect;
    GetClientRect(window, &rect);
    const int width = rect.right, height = rect.bottom;
    if (!renderBuffer.Ensure(width, height)) return;
    {
        Bitmap surface(width, height, width * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(renderBuffer.pixels));
        Graphics g(&surface);
        g.Clear(Color(0, 0, 0, 0));
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.ScaleTransform(width / static_cast<float>(artworkWidth), height / static_cast<float>(artworkHeight));
        SYSTEMTIME now;
        GetLocalTime(&now);
        DrawArtwork(g, now, showDial, showTicks, showNumbers);
        DrawSmashStarEffect(g);
        DrawPatHeartEffect(g);
#ifdef SKEA_TEST_BUILD
        Pen patArea(Color(220, 40, 190, 110), 1.5f);
        g.DrawEllipse(&patArea, clockCenter.X - PatWarpRadius, clockCenter.Y - PatWarpRadius,
            PatWarpRadius * 2, PatWarpRadius * 2);
        SolidBrush headTouch(Color(80, 255, 90, 90));
        g.FillEllipse(&headTouch, clockCenter.X - HeadTouchRadiusX, clockCenter.Y - TouchRadius,
            HeadTouchRadiusX * 2, TouchRadius * 2);
        const PointF cheek = CheekTouchCenter();
        SolidBrush cheekTouch(Color(80, 70, 160, 255));
        g.FillEllipse(&cheekTouch, cheek.X - CheekTouchRadius, cheek.Y - CheekTouchRadius,
            CheekTouchRadius * 2, CheekTouchRadius * 2);
        DrawTestAlarmButton(g);
#endif
        g.ResetTransform();
#ifdef SKEA_TEST_BUILD
        DrawHeadPressRange(g);
#endif
        DrawLayoutEditor(g, width, height);
        g.Flush(FlushIntentionSync);
    }
    SIZE dimensions = { width, height };
    POINT origin = {};
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(window, nullptr, nullptr, &dimensions, renderBuffer.memory,
        &origin, 0, &blend, ULW_ALPHA);
}

// Use elapsed time so delayed window messages do not slow down the GIF.
void AnimateBackground(HWND window) {
    KillTimer(window, 2);
    if (frameEnds.size() < 2) return;
    const ULONGLONG now = GetTickCount64();
    ULONGLONG elapsed = now - animationStart;
    if (alarmActive) {
        if (!alarmPanicActive) {
            if (elapsed < AlarmHandGatherMs) {
                RenderClock(window);
                SetTimer(window, 2, 16, nullptr);
                return;
            }
            alarmPanicActive = true;
            animationStart = now;
            elapsed = 0;
            activeFrame = 0;
            panicImage->SelectFrame(0);
            PlaySoundFile(L"alert.wav");
            RenderClock(window);
        }
        const UINT frame = FrameAt(panicFrameEnds, elapsed);
        if (frame != activeFrame && panicImage->SelectFrame(frame) == Ok) {
            activeFrame = frame;
            RenderClock(window);
        }
        const UINT phase = static_cast<UINT>(elapsed % panicFrameEnds.back());
        SetTimer(window, 2, std::max(1u, panicFrameEnds[frame] - phase), nullptr);
        return;
    }
    if (touchReleaseActive) {
        if (elapsed < touchEndFrameEnds.back()) {
            const UINT frame = FrameAt(touchEndFrameEnds, elapsed);
            if (frame != activeFrame && SelectTouchReleaseFrame(frame)) {
                activeFrame = frame;
                RenderClock(window);
            }
            SetTimer(window, 2, std::max<UINT>(1,
                touchEndFrameEnds[frame] - static_cast<UINT>(elapsed)), nullptr);
            return;
        }
        CompleteTouchRelease(now);
        elapsed = 0;
        RenderClock(window);
    }
    if (cheekDragging) {
        const UINT frame = FrameAt(touchFrameEnds, elapsed);
        if (frame != activeFrame && SelectLayerFrames(frame, *touchBackImage, *touchHeadImage, *touchFrontImage)) {
            activeFrame = frame;
            RenderClock(window);
        }
        const UINT phase = static_cast<UINT>(elapsed % touchFrameEnds.back());
        SetTimer(window, 2, std::max(1u, touchFrameEnds[frame] - phase), nullptr);
        return;
    }
    AnimationImage* previousAnimation = CurrentGif();
    if (patReleaseActive && elapsed >= patEndFrameEnds.back()) {
        patReleaseActive = false;
        animationStart = now;
        idlePhaseOffset = 0;
        elapsed = 0;
    }
    AnimationImage* animation = gifImage.get();
    const std::vector<UINT>* ends = &frameEnds;
    ULONGLONG localElapsed = elapsed + idlePhaseOffset;
    UINT frameOffset = 0;
    if (patActive || patReleaseActive) {
        animation = patActive ? patIdleImage.get() : patEndImage.get();
        ends = patActive ? &patIdleFrameEnds : &patEndFrameEnds;
        localElapsed = elapsed;
    } else if (blankPlaybackActive) {
        animation = blankImage.get();
        ends = &blankFrameEnds;
        localElapsed = elapsed;
    } else if (reactionActive) {
        const ULONGLONG angryDuration = angryFrameEnds.back();
        const ReactionStage stage = ReactionStageAt(elapsed);
        if (stage == ReactionStage::Smash) {
            showingAngry = false;
            showingAngryAlt = false;
            showingTransition = false;
            animation = smashImage.get();
            ends = &smashFrameEnds;
            localElapsed = elapsed + SmashStartOffset();
        } else if (stage == ReactionStage::Angry) {
            showingAngry = true;
            showingAngryAlt = false;
            showingTransition = false;
            animation = angryImage.get();
            ends = &angryFrameEnds;
            localElapsed = (elapsed - AngryStart()) % angryDuration;
        } else if (stage == ReactionStage::AngryAlt) {
            showingAngry = false;
            showingAngryAlt = true;
            showingTransition = false;
            animation = angryAltImage.get();
            ends = &angryAltFrameEnds;
            localElapsed = elapsed - AngryAltStart();
            frameOffset = AngryAltStartFrame;
        } else if (stage == ReactionStage::Transition) {
            if (!showingTransition) {
                handReturnActive = true;
                handReturnStartTurns[0] = handReturnStartTurns[1] = .5f;
                handReturnStartLengthScales[0] = handReturnStartLengthScales[1] = 1.0f;
                handReturnStart = now;
            }
            showingAngry = false;
            showingAngryAlt = false;
            showingTransition = true;
            animation = transitionImage.get();
            ends = &transitionFrameEnds;
            localElapsed = elapsed - TransitionStart();
            frameOffset = TransitionStartFrame;
        } else {
            reactionActive = false;
            showingAngry = false;
            showingAngryAlt = false;
            showingTransition = false;
            animationStart = now;
            idlePhaseOffset = frameEnds[IdleReturnFrame - 1];
            animation = gifImage.get();
            localElapsed = idlePhaseOffset;
        }
    }
    const UINT localFrame = FrameAt(*ends, localElapsed);
    const UINT frame = localFrame + frameOffset;
    bool rendered = false;
    if ((animation != previousAnimation || frame != activeFrame) &&
        animation->SelectFrame(frame) == Ok) {
        activeFrame = frame;
        RenderClock(window);
        rendered = true;
    }
    const bool smashEffectFrame = AdvanceSmashStarEffect(now);
    if (smashEffectFrame && !rendered) RenderClock(window);
    const UINT phase = static_cast<UINT>(localElapsed % ends->back());
    UINT nextFrameDelay = std::max(1u, (*ends)[localFrame] - phase);
    if (smashStarActive) nextFrameDelay = std::min(16u, nextFrameDelay);
    SetTimer(window, 2, nextFrameDelay, nullptr);
}

void StartAlarm(HWND window, const SYSTEMTIME& now) {
    if (GetCapture() == window) ReleaseCapture();
    activeAlarmMinuteKey = AlarmMinuteKey(now);
    BeginAlarm(now);
    StartAlarmSound();
    RenderClock(window);
    AnimateBackground(window);
}

void DismissAlarm(HWND window) {
    if (!alarmActive) return;
    StopAlarmSound();
    DismissAlarmToIdle();
    activeAlarmMinuteKey = ULLONG_MAX;
    RenderClock(window);
    AnimateBackground(window);
}

bool AlarmMinuteHasEnded(const SYSTEMTIME& now) {
    return alarmActive && alarmPanicActive &&
        AlarmMinuteKey(now) != activeAlarmMinuteKey;
}

void StartBlankPlayback(HWND window) {
    StopAlarmSound();
    BeginBlankPlayback();
    PlaySoundFile(L"blank1.wav");
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

void UpdateCheekPull(HWND window, PointF mouse) {
    if (!cheekDragging) return;
    cheekPull = ClampedCheekPull(mouse);
    RenderClock(window);
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

bool IsReactionPoint(float x, float y) {
    return std::hypot((x - clockCenter.X) / HeadTouchRadiusX,
        (y - clockCenter.Y) / TouchRadius) <= 1.0f;
}

bool IsCheekPoint(float x, float y) {
    const PointF center = CheekTouchCenter();
    return std::hypot(x - center.X, y - center.Y) <= CheekTouchRadius;
}

PointF ClickPoint(HWND window, LPARAM position) {
    RECT rect;
    GetClientRect(window, &rect);
    if (rect.right <= 0 || rect.bottom <= 0) return PointF(-1, -1);
    return PointF(GET_X_LPARAM(position) * artworkWidth / static_cast<float>(rect.right),
        GET_Y_LPARAM(position) * artworkHeight / static_cast<float>(rect.bottom));
}

#include "clock-head-interaction.h"

void UpdateClock(HWND window) {
    SYSTEMTIME now;
    GetLocalTime(&now);
    if (AlarmMinuteHasEnded(now)) DismissAlarm(window);
    const unsigned long long minuteKey = AlarmMinuteKey(now);
    if (minuteKey != lastAlarmMinuteKey) {
        for (const Alarm& alarm : alarms) {
            if (AlarmMatches(alarm, now)) {
                lastAlarmMinuteKey = minuteKey;
                if (!alarmActive) StartAlarm(window, now);
                break;
            }
        }
    }
    int step = now.wHour * 3600 + now.wMinute * 60 + now.wSecond;
    if (step == lastStep) return;
    lastStep = step;
    RenderClock(window);
}

HMENU CreateVolumeMenu(int firstCommand, int selectedVolume) {
    HMENU menu = CreatePopupMenu();
    for (int volume = 0; volume <= 100; volume += 10) {
        const std::wstring label = std::to_wstring(volume) + L"%";
        AppendMenuW(menu, MF_STRING | (volume == selectedVolume ? MF_CHECKED : 0),
            firstCommand + volume / 10, label.c_str());
    }
    return menu;
}

void AppendSoundMenus(HMENU menu) {
    HMENU effectVolumeMenu = CreateVolumeMenu(EffectVolumeFirst, effectSoundVolume);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(effectVolumeMenu), UiText().effectVolume);

    HMENU alarmSoundMenu = CreatePopupMenu();
    AppendMenuW(alarmSoundMenu, MF_STRING, AlarmFileChoose, UiText().alarmFileChoose);
    if (!customAlarmSoundPath.empty()) {
        const std::wstring name = FileName(customAlarmSoundPath);
        AppendMenuW(alarmSoundMenu, MF_STRING | (useCustomAlarmSound ? MF_CHECKED : 0),
            AlarmSoundCustom, name.c_str());
    }
    AppendMenuW(alarmSoundMenu, MF_STRING | (!useCustomAlarmSound ? MF_CHECKED : 0),
        AlarmSoundDefault, UiText().alarmDefault);
    AppendMenuW(alarmSoundMenu, MF_SEPARATOR, 0, nullptr);
    HMENU alarmVolumeMenu = CreateVolumeMenu(AlarmVolumeFirst, alarmSoundVolume);
    AppendMenuW(alarmSoundMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(alarmVolumeMenu), UiText().volume);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(alarmSoundMenu), UiText().alarmSound);
}

void ShowMenu(HWND window, POINT point) {
    DismissAlarm(window);
    if (exceptionMenu) return;
    ReloadCustomLanguage(window, settingsPath);
    MenuIcons icons = { GetDpiForWindow(window), {} };
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"Skea clock");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (showDial ? MF_CHECKED : 0), Background, UiText().background);
    AppendMenuW(menu, MF_STRING | (showTicks ? MF_CHECKED : 0), Ticks, UiText().ticks);
    AppendMenuW(menu, MF_STRING | (showNumbers ? MF_CHECKED : 0), Numbers, UiText().numbers);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, AlarmAddCommand, UiText().alarmAdd);
    icons.SetLast(menu, MAKEINTRESOURCEW(SKEA_MENU_BELL));
    for (size_t i = 0; i < alarms.size(); ++i) {
        HMENU alarmMenu = CreatePopupMenu();
        AppendMenuW(alarmMenu, MF_STRING, AlarmEditFirst + static_cast<UINT>(i), UiText().alarmEdit);
        AppendMenuW(alarmMenu, MF_STRING, AlarmDeleteFirst + static_cast<UINT>(i), UiText().alarmDelete);
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(alarmMenu), AlarmMenuLabel(alarms[i]).c_str());
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendSoundMenus(menu);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (topmost ? MF_CHECKED : 0), AlwaysOnTop, UiText().alwaysOnTop);
    AppendTopmostExceptionMenu(menu);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (IsStartupEnabled() ? MF_CHECKED : 0), RunAtStartup, UiText().runAtStartup);
    AppendMenuW(menu, MF_STRING | (showTaskbar ? MF_CHECKED : 0), Taskbar, UiText().taskbar);
    AppendMenuW(menu, MF_STRING | (layoutEditing ? MF_CHECKED : 0), EditLayout, UiText().layout);
    icons.SetLast(menu, MAKEINTRESOURCEW(SKEA_MENU_MOVE));
    HMENU displayMenu = CreatePopupMenu();
    const auto displays = GetDisplays();
    for (size_t i = 0; i < displays.size() && i < MaxDisplays; ++i) {
        const UINT flags = MF_STRING | (_wcsicmp(displays[i].info.szDevice, activeDisplay.c_str()) == 0 ? MF_CHECKED : 0);
        AppendMenuW(displayMenu, flags, DisplayFirst + static_cast<UINT>(i), DisplayLabel(displays[i]).c_str());
    }
    AppendMenuW(menu, MF_POPUP | (displays.empty() ? MF_GRAYED : 0),
        reinterpret_cast<UINT_PTR>(displayMenu), UiText().displaySelect);
    icons.SetLast(menu, MAKEINTRESOURCEW(SKEA_MENU_VM));
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(CreateLanguageMenu()), L"Language");
    icons.SetLast(menu, MAKEINTRESOURCEW(SKEA_MENU_GLOBE));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (autoCheckUpdates ? MF_CHECKED : 0), AutoUpdateCheck, UiText().autoUpdateCheck);
    AppendMenuW(menu, MF_STRING, GithubVisit, UiText().githubVisit);
    icons.SetLast(menu, MAKEINTRESOURCEW(SKEA_MENU_GITHUB));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, Exit, UiText().exit);
    icons.SetLast(menu, MAKEINTRESOURCEW(SKEA_MENU_CLOSE));
    SetForegroundWindow(window);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        point.x, point.y, 0, window, nullptr);
    DestroyMenu(menu);
    CloseExceptionMenu();
    if (command) SendMessageW(window, WM_COMMAND, command, 0);
    PostMessageW(window, WM_NULL, 0, 0);
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
    case WM_TIMER:
        if (wParam == 2) { AnimateBackground(window); return 0; }
        if (wParam == PatHeartTimer) {
            const bool active = AdvancePatHeartEffect(GetTickCount64());
            RenderClock(window);
            if (!active) KillTimer(window, PatHeartTimer);
            return 0;
        }
        if (wParam == HeadHoldTimer) {
            KillTimer(window, HeadHoldTimer);
            if (headPressPending && GetCapture() == window) {
                StartPat(window);
            }
            return 0;
        }
        RecoverTopmostState(window);
        UpdateClock(window);
        CheckAlarmSoundValidation(window);
        CheckUpdateResult(window);
        return 0;
    case WM_TIMECHANGE:
        lastStep = -1;
        lastAlarmMinuteKey = ULLONG_MAX;
        UpdateClock(window);
        return 0;
    case WM_LBUTTONDOWN:
        {
            const PointF point = ClickPoint(window, lParam);
#ifdef SKEA_TEST_BUILD
            if (TestAlarmButtonBounds.Contains(point)) {
                layoutEditing = false;
                SYSTEMTIME now;
                GetLocalTime(&now);
                StartAlarm(window, now);
                return 0;
            }
#endif
            if (layoutEditing) {
                RECT client;
                GetClientRect(window, &client);
                const LayoutHandle handle = LayoutHitAt(client.right, client.bottom,
                    static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                if (handle == LayoutHandle::Close) {
                    layoutEditing = false;
                    SaveCurrentLayout(window);
                    StopBlankPlayback(window);
                    PlaySoundFile(L"answer.wav");
                } else if (handle != LayoutHandle::None) {
                    BeginLayoutDrag(window, handle);
                }
                return 0;
            }
            if (IsReactionPoint(point.X, point.Y)) {
                headPressPending = true;
                headPressStart = GetTickCount64();
                headPressPoint = point;
                headPressMouse = point;
                headPressClientPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                SetCapture(window);
                SetTimer(window, HeadHoldTimer, PatHoldMs, nullptr);
#ifdef SKEA_TEST_BUILD
                RenderClock(window);
#endif
            }
            else if (IsCheekPoint(point.X, point.Y)) StartCheekPull(window);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (layoutEditing) {
            if (layoutDrag != LayoutHandle::None) {
                UpdateLayoutDrag(window);
                RenderClock(window);
            } else {
                RECT client;
                GetClientRect(window, &client);
                SetCursor(LayoutCursor(LayoutHitAt(client.right, client.bottom,
                    static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)))));
            }
            return 0;
        }
        if (wParam & MK_LBUTTON) UpdateHeadPress(window, lParam);
        if (cheekDragging) UpdateCheekPull(window, ClickPoint(window, lParam));
        return 0;
    case WM_LBUTTONUP:
        if (layoutEditing && layoutDrag != LayoutHandle::None) {
            FinishLayoutDrag(window, true);
            return 0;
        }
        if (headPressPending || patActive) {
            UpdateHeadPress(window, lParam);
            FinishHeadPress(window, true);
            if (GetCapture() == window) ReleaseCapture();
            return 0;
        }
        if (cheekDragging) {
            ReleaseCapture();
            FinishCheekPull(window);
        }
        return 0;
    case WM_CAPTURECHANGED:
    case WM_CANCELMODE: {
        const bool headPressCaptured = headPressPending || patActive;
        if (layoutDrag != LayoutHandle::None) FinishLayoutDrag(window, false);
        FinishHeadPress(window, false);
        if (message == WM_CANCELMODE && headPressCaptured && GetCapture() == window) ReleaseCapture();
        FinishCheekPull(window);
        return 0;
    }
    case WM_SETCURSOR:
        if (layoutEditing && LOWORD(lParam) == HTCLIENT) {
            POINT cursor;
            RECT client;
            GetCursorPos(&cursor);
            ScreenToClient(window, &cursor);
            GetClientRect(window, &client);
            SetCursor(LayoutCursor(LayoutHitAt(client.right, client.bottom,
                static_cast<float>(cursor.x), static_cast<float>(cursor.y))));
            return TRUE;
        }
        break;
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DPICHANGED:
        PositionClock(window);
        RenderClock(window);
        return 0;
    case WM_CONTEXTMENU: {
        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (point.x == -1 && point.y == -1) GetCursorPos(&point);
        ShowMenu(window, point);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == TopmostExceptionAdd) {
            if (ChooseTopmostException(window)) {
                SaveSettings();
                ApplyTopmostState(window, true);
            }
            return 0;
        }
        if (LOWORD(wParam) >= TopmostExceptionFirst &&
            LOWORD(wParam) < TopmostExceptionFirst + static_cast<int>(topmostExceptionMenuPaths.size())) {
            const size_t index = LOWORD(wParam) - TopmostExceptionFirst;
            if (ToggleTopmostException(topmostExceptionMenuPaths[index])) {
                SaveSettings();
                ApplyTopmostState(window, true);
            }
            return 0;
        }
        if (LOWORD(wParam) >= EffectVolumeFirst && LOWORD(wParam) <= EffectVolumeFirst + 10) {
            effectSoundVolume = (LOWORD(wParam) - EffectVolumeFirst) * 10;
            SaveSettings();
            return 0;
        }
        if (LOWORD(wParam) >= AlarmVolumeFirst && LOWORD(wParam) <= AlarmVolumeFirst + 10) {
            alarmSoundVolume = (LOWORD(wParam) - AlarmVolumeFirst) * 10;
            SaveSettings();
            return 0;
        }
        if (LOWORD(wParam) == AlarmFileChoose) {
            if (ChooseAlarmSoundFile(window)) {
                SaveSettings();
                BeginAlarmSoundValidation(window);
            }
            return 0;
        }
        if (LOWORD(wParam) == AlarmSoundDefault) {
            useCustomAlarmSound = false;
            SaveSettings();
            BeginAlarmSoundValidation(window);
            return 0;
        }
        if (LOWORD(wParam) == AlarmSoundCustom) {
            useCustomAlarmSound = !customAlarmSoundPath.empty();
            SaveSettings();
            BeginAlarmSoundValidation(window);
            return 0;
        }
        if (LOWORD(wParam) == AlarmAddCommand) {
            layoutEditing = false;
            StartBlankPlayback(window);
            OpenAlarmEditor(window);
            if (!alarmEditorWindow) StopBlankPlayback(window);
            return 0;
        }
        if (LOWORD(wParam) >= AlarmEditFirst &&
            LOWORD(wParam) < AlarmEditFirst + static_cast<int>(alarms.size())) {
            const size_t index = LOWORD(wParam) - AlarmEditFirst;
            const Alarm alarm = alarms[index];
            alarms.erase(alarms.begin() + index);
            SaveAlarms();
            layoutEditing = false;
            StartBlankPlayback(window);
            OpenAlarmEditor(window, &alarm);
            if (!alarmEditorWindow) StopBlankPlayback(window);
            return 0;
        }
        if (LOWORD(wParam) >= AlarmDeleteFirst &&
            LOWORD(wParam) < AlarmDeleteFirst + static_cast<int>(alarms.size())) {
            const size_t index = LOWORD(wParam) - AlarmDeleteFirst;
            alarms.erase(alarms.begin() + index);
            SaveAlarms();
            return 0;
        }
        if (LOWORD(wParam) == EditLayout) {
            layoutEditing = true;
            StartBlankPlayback(window);
            return 0;
        }
        if (LOWORD(wParam) >= DisplayFirst && LOWORD(wParam) < DisplayFirst + MaxDisplays) {
            const auto displays = GetDisplays();
            const int index = LOWORD(wParam) - DisplayFirst;
            if (index < static_cast<int>(displays.size())) {
                SaveCurrentLayout(window);
                selectedDisplay = displays[index].info.szDevice;
                PositionClock(window);
                SaveSettings();
                RenderClock(window);
            }
            return 0;
        }
        if (LOWORD(wParam) >= LanguageFileChoose &&
            LOWORD(wParam) < LanguageFirst + static_cast<int>(std::size(LanguagePacks))) {
            if (LOWORD(wParam) == LanguageFileChoose) {
                if (!ChooseLanguageFile(window)) return 0;
            } else if (LOWORD(wParam) == LanguageCustom) {
                if (!ReloadCustomLanguage(window, settingsPath)) return 0;
                useCustomLanguage = true;
                selectedLanguage = customLanguage->strings.languageCode;
            } else {
                useCustomLanguage = false;
                selectedLanguage = LanguagePacks[LOWORD(wParam) - LanguageFirst].code;
            }
            SaveSettings();
            lastStep = -1;
            if (alarmEditorWindow) {
                SetWindowTextW(alarmEditorWindow, UiText().alarmAdd);
                InvalidateRect(alarmEditorWindow, nullptr, TRUE);
            }
            PlaySoundFile(L"smile.wav");
            return 0;
        }
        switch (LOWORD(wParam)) {
        case Background: showDial = !showDial; break;
        case Ticks: showTicks = !showTicks; break;
        case Numbers: showNumbers = !showNumbers; break;
        case AlwaysOnTop: topmost = !topmost; ApplyTopmostState(window, true); break;
        case Taskbar: showTaskbar = !showTaskbar; ApplyTaskbarSetting(window); break;
        case RunAtStartup:
            if (SetStartupEnabled(!IsStartupEnabled()) != ERROR_SUCCESS)
                MessageBoxW(window, UiText().startupError, L"Skea clock", MB_OK | MB_ICONERROR);
            return 0;
        case AutoUpdateCheck: autoCheckUpdates = !autoCheckUpdates; break;
        case GithubVisit:
            ShellExecuteW(window, L"open", GithubProjectUrl, nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        case Exit: DestroyWindow(window); return 0;
        default: return 0;
        }
        SaveSettings();
        RenderClock(window);
        return 0;
    case WM_CLOSE:
        SaveCurrentLayout(window);
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, 1);
        KillTimer(window, 2);
        KillTimer(window, HeadHoldTimer);
        KillTimer(window, PatHeartTimer);
        StopForegroundMonitor();
        CancelAlarmSoundValidation();
        updateCheck.reset();
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
        HWND existing = FindWindowW(ClassName, L"Skea clock");
        if (existing) ShowWindow(existing, SW_SHOWNOACTIVATE);
        CloseHandle(mutex);
        return 0;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    wchar_t executable[MAX_PATH];
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    settingsPath = executable;
    settingsPath = settingsPath.substr(0, settingsPath.find_last_of(L"\\/") + 1) + L"skea-clock.ini";
    showDial = GetPrivateProfileIntW(L"clock", L"background", 1, settingsPath.c_str()) != 0;
    topmost = GetPrivateProfileIntW(L"clock", L"topmost", 1, settingsPath.c_str()) != 0;
    showTicks = GetPrivateProfileIntW(L"clock", L"ticks", 1, settingsPath.c_str()) != 0;
    showNumbers = GetPrivateProfileIntW(L"clock", L"numbers", 1, settingsPath.c_str()) != 0;
    LoadLanguageSettings(settingsPath);
    LoadSoundSettings(settingsPath);
    LoadWindowBehaviorSettings(settingsPath);
    autoCheckUpdates = GetPrivateProfileIntW(L"updates", L"enabled", 1, settingsPath.c_str()) != 0;
    wchar_t display[MAX_PATH] = {};
    GetPrivateProfileStringW(L"clock", L"display", L"", display, MAX_PATH, settingsPath.c_str());
    selectedDisplay = display;
    LoadAlarms();

    GdiplusStartupInput input;
    ULONG_PTR token;
    if (GdiplusStartup(&token, &input, nullptr) != Ok) {
        CloseHandle(mutex);
        return 1;
    }
    std::wstring directory = settingsPath.substr(0, settingsPath.find_last_of(L"\\/") + 1);
    std::wstring sourceDirectory = directory + L"sources\\";
    if (GetFileAttributesW((sourceDirectory + L"locate data\\clock-layout.ini").c_str()) == INVALID_FILE_ATTRIBUTES)
        sourceDirectory = directory + L"..\\sources\\";
    SetSoundDirectory(sourceDirectory);
    if (!LoadArtwork(sourceDirectory)) {
        const std::wstring error = std::wstring(UiText().artworkLoadError) + L"\n\n" + UiText().artworkLoadHelp;
        MessageBoxW(nullptr, error.c_str(), L"Skea clock", MB_OK | MB_ICONERROR);
        ReleaseArtwork();
        GdiplusShutdown(token);
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
        PlayRandomIdleSound();
        UpdateClock(window);
        ShowWindow(window, SW_SHOWNOACTIVATE);
        StartForegroundMonitor(window);
        ApplyTopmostState(window, true);
        BeginAlarmSoundValidation(window);
        BeginUpdateCheck();
        MSG message;
        int status;
        while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        result = status == -1 ? 1 : static_cast<int>(message.wParam);
    } else {
        MessageBoxW(nullptr, UiText().clockWindowError, L"Skea clock", MB_OK | MB_ICONERROR);
    }
    renderBuffer.Release();
    ReleaseArtwork();
    GdiplusShutdown(token);
    CloseHandle(mutex);
    return result;
}
