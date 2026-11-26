#pragma once

#include "../window/window-render.h"

constexpr size_t MaximumAlarms = 64;

unsigned long long lastAlarmMinuteKey = ULLONG_MAX;
unsigned long long activeAlarmMinuteKey = ULLONG_MAX;

struct Alarm {
    int hour = 0;
    int minute = 0;
    unsigned days = 0x7f;
};

std::vector<Alarm> alarms;

std::wstring FormatAlarmTime(const Alarm& alarm) {
    wchar_t text[16] = {};
    swprintf_s(text, L"%02d:%02d", alarm.hour, alarm.minute);
    return text;
}

int AlarmDayBitFromSystemDay(WORD dayOfWeek) {
    return dayOfWeek == 0 ? 6 : dayOfWeek - 1;
}

bool AlarmMatches(const Alarm& alarm, const SYSTEMTIME& time) {
    return alarm.hour == time.wHour && alarm.minute == time.wMinute &&
        (alarm.days & (1u << AlarmDayBitFromSystemDay(time.wDayOfWeek))) != 0;
}

unsigned long long AlarmMinuteKey(const SYSTEMTIME& time) {
    return (((static_cast<unsigned long long>(time.wYear) * 13 + time.wMonth) * 32 + time.wDay) * 24 +
        time.wHour) * 60 + time.wMinute;
}

void InitializeAlarmMinuteTracking(const SYSTEMTIME& now) {
    lastAlarmMinuteKey = AlarmMinuteKey(now);
}

void StartAlarm(HWND window, const SYSTEMTIME& now) {
    if (IsKommyCharacter()) return;
    if (GetCapture() == window) ReleaseCapture();
    activeAlarmMinuteKey = AlarmMinuteKey(now);
    BeginAlarm(now);
    StartClockAlarmSound();
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

bool AlarmAnimationDueSoon(const SYSTEMTIME& now) {
    if (alarms.empty()) return false;
    if (AlarmMinuteKey(now) != lastAlarmMinuteKey &&
        std::any_of(alarms.begin(), alarms.end(), [&](const Alarm& alarm) { return AlarmMatches(alarm, now); }))
        return true;

    // Request at 11 seconds to leave room for the window timer before the 10-second mark.
    if (now.wSecond < 49) return false;
    const int minute = (now.wHour * 60 + now.wMinute + 1) % (24 * 60);
    SYSTEMTIME next = now;
    next.wHour = static_cast<WORD>(minute / 60);
    next.wMinute = static_cast<WORD>(minute % 60);
    if (minute == 0) next.wDayOfWeek = static_cast<WORD>((now.wDayOfWeek + 1) % 7);
    return std::any_of(alarms.begin(), alarms.end(), [&](const Alarm& alarm) { return AlarmMatches(alarm, next); });
}

void UpdateAlarm(HWND window, const SYSTEMTIME& now) {
    prepareAlarmAnimation = !IsKommyCharacter() && AlarmAnimationDueSoon(now);
    if (AlarmMinuteHasEnded(now)) DismissAlarm(window);
    const unsigned long long minuteKey = AlarmMinuteKey(now);
    if (!IsKommyCharacter() && minuteKey != lastAlarmMinuteKey) {
        for (const Alarm& alarm : alarms) {
            if (AlarmMatches(alarm, now)) {
                lastAlarmMinuteKey = minuteKey;
                if (!alarmActive) StartAlarm(window, now);
                break;
            }
        }
    }
}
