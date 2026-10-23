#pragma once

constexpr int AlarmAddCommand = 108;
constexpr int AlarmEditFirst = 1000;
constexpr int AlarmDeleteFirst = 2000;
constexpr size_t MaximumAlarms = 64;
constexpr wchar_t AlarmEditorClassName[] = L"SkeaAlarmEditorWindow";
constexpr UINT AlarmEditorClosedMessage = WM_APP + 2;

struct Alarm {
    int hour = 0;
    int minute = 0;
    unsigned days = 0x7f;
};

std::vector<Alarm> alarms;
HWND alarmEditorWindow = nullptr;
HWND alarmHourEdit = nullptr;
HWND alarmMinuteEdit = nullptr;
WNDPROC alarmOriginalEditProc = nullptr;
HFONT alarmTimeFont = nullptr;
HBRUSH alarmBackgroundBrush = nullptr;
Alarm alarmDraft;
UINT alarmEditorDpi = 96;

std::wstring FormatAlarmTime(const Alarm& alarm) {
    wchar_t text[16] = {};
    swprintf_s(text, L"%02d:%02d", alarm.hour, alarm.minute);
    return text;
}

std::wstring AlarmDaysText(unsigned days) {
    const auto& labels = UiText().alarmDays;
    constexpr int bits[] = { 6, 0, 1, 2, 3, 4, 5 };
    std::wstring text;
    for (int i = 0; i < 7; ++i) {
        if (!(days & (1u << bits[i]))) continue;
        if (!text.empty()) text += L" ";
        text += labels[i];
    }
    return text.empty() ? UiText().alarmNoDays : text;
}

std::wstring AlarmMenuLabel(const Alarm& alarm) {
    return FormatAlarmTime(alarm) + L" · " + AlarmDaysText(alarm.days);
}

bool ParseAlarmPart(const std::wstring& value, int maximum, int& result) {
    if (value.empty() || value.size() > 2) return false;
    for (wchar_t character : value) if (character < L'0' || character > L'9') return false;
    result = std::stoi(value);
    return result <= maximum;
}

void SaveAlarms() {
    const int previousCount = GetPrivateProfileIntW(L"alarms", L"count", 0, settingsPath.c_str());
    WritePrivateProfileStringW(L"alarms", L"count", std::to_wstring(alarms.size()).c_str(), settingsPath.c_str());
    for (size_t i = 0; i < alarms.size(); ++i) {
        const std::wstring key = L"alarm" + std::to_wstring(i);
        const std::wstring value = FormatAlarmTime(alarms[i]) + L"," + std::to_wstring(alarms[i].days);
        WritePrivateProfileStringW(L"alarms", key.c_str(), value.c_str(), settingsPath.c_str());
    }
    for (int i = static_cast<int>(alarms.size()); i < previousCount; ++i) {
        const std::wstring key = L"alarm" + std::to_wstring(i);
        WritePrivateProfileStringW(L"alarms", key.c_str(), nullptr, settingsPath.c_str());
    }
}

void LoadAlarms() {
    alarms.clear();
    const int count = std::clamp(static_cast<int>(GetPrivateProfileIntW(
        L"alarms", L"count", 0, settingsPath.c_str())),
        0, static_cast<int>(MaximumAlarms));
    for (int i = 0; i < count; ++i) {
        wchar_t value[64] = {};
        const std::wstring key = L"alarm" + std::to_wstring(i);
        GetPrivateProfileStringW(L"alarms", key.c_str(), L"", value, 64, settingsPath.c_str());
        int hour = -1, minute = -1;
        unsigned days = 0;
        if (swscanf_s(value, L"%d:%d,%u", &hour, &minute, &days) == 3 &&
            hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && days > 0 && days <= 0x7f)
            alarms.push_back({ hour, minute, days });
    }
}

RECT AlarmEditorBounds(const RECT& clock, const RECT& work, int width, int height, int gap) {
    RECT bounds = {};
    bounds.left = clock.left + ((clock.right - clock.left) - width) / 2;
    bounds.left = std::clamp(bounds.left, work.left, std::max(work.left, work.right - width));
    bounds.top = clock.top - height - gap;
    if (bounds.top < work.top) bounds.top = clock.bottom + gap;
    bounds.top = std::clamp(bounds.top, work.top, std::max(work.top, work.bottom - height));
    bounds.right = bounds.left + width;
    bounds.bottom = bounds.top + height;
    return bounds;
}

PointF AlarmCheckCenter(int width, int height) {
    return PointF(width - static_cast<float>(MulDiv(52, alarmEditorDpi, 96)), height * .25f);
}
PointF AlarmCloseCenter(int width, int height) {
    return PointF(width - static_cast<float>(MulDiv(18, alarmEditorDpi, 96)), height * .25f);
}
PointF AlarmDayCenter(int index, int width, int height) {
    return PointF((index + .5f) * width / 7.0f, height * .75f);
}

int AlarmDayBit(int displayIndex) {
    constexpr int bits[] = { 6, 0, 1, 2, 3, 4, 5 };
    return bits[displayIndex];
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

#include "alarm-editor-font.h"

void LayoutAlarmEditor(HWND window) {
    RECT client;
    GetClientRect(window, &client);
    const int width = client.right, height = client.bottom;
    const int margin = MulDiv(14, alarmEditorDpi, 96);
    const int actionSpace = MulDiv(82, alarmEditorDpi, 96);
    const int editHeight = MulDiv(32, alarmEditorDpi, 96);
    const int editWidth = MulDiv(48, alarmEditorDpi, 96);
    const int editGap = MulDiv(18, alarmEditorDpi, 96);
    const int timeAreaWidth = width - actionSpace;
    const int groupWidth = editWidth * 2 + editGap;
    const int groupLeft = std::max(margin, (timeAreaWidth - groupWidth) / 2);
    const int editTop = height / 4 - editHeight / 2;
    SetWindowPos(alarmHourEdit, nullptr, groupLeft, editTop, editWidth, editHeight,
        SWP_NOACTIVATE | SWP_NOZORDER);
    SetWindowPos(alarmMinuteEdit, nullptr, groupLeft + editWidth + editGap, editTop,
        editWidth, editHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    const int radius = MulDiv(18, alarmEditorDpi, 96);
    SetWindowRgn(window, CreateRoundRectRgn(0, 0, width + 1, height + 1, radius, radius), TRUE);
}

void RegisterDraftAlarm(HWND window);

LRESULT CALLBACK AlarmTimeEditProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CHAR) {
        if (window == alarmHourEdit && (wParam == VK_TAB || wParam == VK_RETURN || wParam == VK_SPACE)) {
            SetFocus(alarmMinuteEdit);
            SendMessageW(alarmMinuteEdit, EM_SETSEL, 0, -1);
            return 0;
        }
        if (window == alarmMinuteEdit && wParam == VK_RETURN) {
            RegisterDraftAlarm(alarmEditorWindow);
            return 0;
        }
        const LRESULT result = CallWindowProcW(alarmOriginalEditProc, window, message, wParam, lParam);
        if (window == alarmHourEdit && wParam >= L'0' && wParam <= L'9' &&
            GetWindowTextLengthW(alarmHourEdit) >= 2) {
            SetFocus(alarmMinuteEdit);
            SendMessageW(alarmMinuteEdit, EM_SETSEL, 0, -1);
        }
        return result;
    }
    return CallWindowProcW(alarmOriginalEditProc, window, message, wParam, lParam);
}

void RegisterDraftAlarm(HWND window) {
    wchar_t hourValue[8] = {}, minuteValue[8] = {};
    GetWindowTextW(alarmHourEdit, hourValue, 8);
    GetWindowTextW(alarmMinuteEdit, minuteValue, 8);
    int hour = 0, minute = 0;
    if (!ParseAlarmPart(hourValue, 23, hour) || !ParseAlarmPart(minuteValue, 59, minute) ||
        alarmDraft.days == 0 || alarms.size() >= MaximumAlarms) {
        MessageBeep(MB_ICONWARNING);
        HWND invalid = ParseAlarmPart(hourValue, 23, hour) ? alarmMinuteEdit : alarmHourEdit;
        SetFocus(invalid);
        SendMessageW(invalid, EM_SETSEL, 0, -1);
        return;
    }
    alarmDraft.hour = hour;
    alarmDraft.minute = minute;
    alarms.push_back(alarmDraft);
    SaveAlarms();
    SYSTEMTIME registeredAt;
    GetLocalTime(&registeredAt);
    InitializeAlarmMinuteTracking(registeredAt);
    PlaySoundFile(L"answer.wav");
    DestroyWindow(window);
}

LRESULT CALLBACK AlarmEditorProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        alarmBackgroundBrush = CreateSolidBrush(RGB(0xDF, 0xF7, 0xF5));
        wchar_t hour[3] = {}, minute[3] = {};
        swprintf_s(hour, L"%02d", alarmDraft.hour);
        swprintf_s(minute, L"%02d", alarmDraft.minute);
        alarmHourEdit = CreateWindowExW(0, L"EDIT", hour,
            WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER, 0, 0, 0, 0,
            window, nullptr, GetModuleHandleW(nullptr), nullptr);
        alarmMinuteEdit = CreateWindowExW(0, L"EDIT", minute,
            WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER, 0, 0, 0, 0,
            window, nullptr, GetModuleHandleW(nullptr), nullptr);
        alarmMenuFont = ReadAlarmMenuFont();
        HDC dc = GetDC(window);
        alarmTimeFont = CreateFittedAlarmFont(dc, nullptr, MulDiv(24, alarmEditorDpi, 96),
            MulDiv(48, alarmEditorDpi, 96) - 4, MulDiv(32, alarmEditorDpi, 96) - 2);
        if (dc) ReleaseDC(window, dc);
        SendMessageW(alarmHourEdit, EM_SETLIMITTEXT, 2, 0);
        SendMessageW(alarmMinuteEdit, EM_SETLIMITTEXT, 2, 0);
        SendMessageW(alarmHourEdit, WM_SETFONT, reinterpret_cast<WPARAM>(alarmTimeFont), TRUE);
        SendMessageW(alarmMinuteEdit, WM_SETFONT, reinterpret_cast<WPARAM>(alarmTimeFont), TRUE);
        alarmOriginalEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            alarmHourEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(AlarmTimeEditProc)));
        SetWindowLongPtrW(alarmMinuteEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(AlarmTimeEditProc));
        LayoutAlarmEditor(window);
        SetFocus(alarmHourEdit);
        SendMessageW(alarmHourEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_SIZE:
        LayoutAlarmEditor(window);
        InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, RGB(50, 63, 72));
        SetBkColor(dc, RGB(0xDF, 0xF7, 0xF5));
        return reinterpret_cast<LRESULT>(alarmBackgroundBrush);
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);
        Graphics graphics(dc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        SolidBrush background(Color(255, 0xDF, 0xF7, 0xF5));
        SolidBrush selected(Color(245, 255, 255, 255));
        Pen separator(Color(190, 255, 255, 255), 2.0f);
        Pen checkPen(Color(255, 70, 83, 92), 2.5f);
        Pen closePen(Color(255, 110, 120, 126), 2.3f);
        graphics.FillRectangle(&background, 0, 0, client.right, client.bottom);
        graphics.DrawLine(&separator, 10.0f, client.bottom * .5f,
            client.right - 10.0f, client.bottom * .5f);
        const float dayRadius = std::min(14.0f * alarmEditorDpi / 96.0f,
            client.right / 7.0f * .4f);
        for (int i = 0; i < 7; ++i) {
            const PointF center = AlarmDayCenter(i, client.right, client.bottom);
            if (alarmDraft.days & (1u << AlarmDayBit(i))) graphics.FillEllipse(&selected,
                center.X - dayRadius, center.Y - dayRadius, dayRadius * 2, dayRadius * 2);
        }
        const PointF check = AlarmCheckCenter(client.right, client.bottom);
        const PointF close = AlarmCloseCenter(client.right, client.bottom);
        const float checkRadius = 14.0f * alarmEditorDpi / 96.0f;
        graphics.FillEllipse(&selected, check.X - checkRadius, check.Y - checkRadius,
            checkRadius * 2, checkRadius * 2);
        graphics.FillEllipse(&selected, close.X - checkRadius, close.Y - checkRadius,
            checkRadius * 2, checkRadius * 2);
        const PointF checkPoints[] = {
            PointF(check.X - 6, check.Y), PointF(check.X - 1, check.Y + 5),
            PointF(check.X + 7, check.Y - 6) };
        graphics.DrawLines(&checkPen, checkPoints, 3);
        graphics.DrawLine(&closePen, close.X - 7, close.Y - 5, close.X + 7, close.Y - 5);
        graphics.DrawLine(&closePen, close.X - 2, close.Y - 7, close.X + 2, close.Y - 7);
        const PointF binOutline[] = {
            PointF(close.X - 5, close.Y - 3), PointF(close.X - 4, close.Y + 7),
            PointF(close.X + 4, close.Y + 7), PointF(close.X + 5, close.Y - 3) };
        graphics.DrawLines(&closePen, binOutline, 4);
        graphics.DrawLine(&closePen, close.X - 2, close.Y, close.X - 2, close.Y + 4);
        graphics.DrawLine(&closePen, close.X + 2, close.Y, close.X + 2, close.Y + 4);
        HDC textDc = graphics.GetHDC();
        if (textDc) {
            DrawAlarmEditorText(textDc, client);
            graphics.ReleaseHDC(textDc);
        }
        EndPaint(window, &paint);
        return 0;
    }
    case WM_LBUTTONUP: {
        RECT client;
        GetClientRect(window, &client);
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        const PointF close = AlarmCloseCenter(client.right, client.bottom);
        if (std::hypot(x - close.X, y - close.Y) <= MulDiv(22, alarmEditorDpi, 96)) {
            DestroyWindow(window);
            return 0;
        }
        const PointF check = AlarmCheckCenter(client.right, client.bottom);
        if (std::hypot(x - check.X, y - check.Y) <= MulDiv(22, alarmEditorDpi, 96)) {
            RegisterDraftAlarm(window);
            return 0;
        }
        for (int i = 0; i < 7; ++i) {
            const PointF center = AlarmDayCenter(i, client.right, client.bottom);
            if (std::hypot(x - center.X, y - center.Y) <= MulDiv(20, alarmEditorDpi, 96)) {
                alarmDraft.days ^= 1u << AlarmDayBit(i);
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
        }
        return 0;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        {
        HWND owner = GetWindow(window, GW_OWNER);
        if (alarmOriginalEditProc) {
            if (alarmHourEdit) SetWindowLongPtrW(alarmHourEdit, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(alarmOriginalEditProc));
            if (alarmMinuteEdit) SetWindowLongPtrW(alarmMinuteEdit, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(alarmOriginalEditProc));
        }
        if (alarmTimeFont) DeleteObject(alarmTimeFont);
        if (alarmBackgroundBrush) DeleteObject(alarmBackgroundBrush);
        alarmTimeFont = nullptr;
        alarmBackgroundBrush = nullptr;
        alarmHourEdit = nullptr;
        alarmMinuteEdit = nullptr;
        alarmOriginalEditProc = nullptr;
        alarmEditorWindow = nullptr;
        if (owner) PostMessageW(owner, AlarmEditorClosedMessage, 0, 0);
        return 0;
        }
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void OpenAlarmEditor(HWND owner, const Alarm* initial = nullptr) {
    if (alarmEditorWindow) DestroyWindow(alarmEditorWindow);
    if (initial) alarmDraft = *initial;
    else {
        SYSTEMTIME now;
        GetLocalTime(&now);
        alarmDraft = { now.wHour, now.wMinute, 0x7f };
    }
    static bool registered = false;
    if (!registered) {
        WNDCLASSW editorClass = {};
        editorClass.lpfnWndProc = AlarmEditorProc;
        editorClass.hInstance = GetModuleHandleW(nullptr);
        editorClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        editorClass.lpszClassName = AlarmEditorClassName;
        if (!RegisterClassW(&editorClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;
        registered = true;
    }
    RECT clock, work;
    GetWindowRect(owner, &clock);
    MONITORINFO monitor = { sizeof(MONITORINFO) };
    GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor);
    work = monitor.rcWork;
    alarmEditorDpi = GetDpiForWindow(owner);
    const int minimumWidth = MulDiv(240, alarmEditorDpi, 96);
    const int workWidth = static_cast<int>(work.right - work.left);
    const int clockWidth = static_cast<int>(clock.right - clock.left);
    const int clockHeight = static_cast<int>(clock.bottom - clock.top);
    const int width = std::min(workWidth, std::max(std::min(clockWidth, clockHeight), minimumWidth));
    const int height = MulDiv(96, alarmEditorDpi, 96);
    const int gap = MulDiv(8, alarmEditorDpi, 96);
    const RECT bounds = AlarmEditorBounds(clock, work, width, height, gap);
    alarmEditorWindow = CreateWindowExW(WS_EX_TOOLWINDOW, AlarmEditorClassName, UiText().alarmAdd,
        WS_POPUP, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
        owner, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!alarmEditorWindow) return;
    SetWindowPos(alarmEditorWindow, topmost ? HWND_TOPMOST : HWND_TOP, bounds.left, bounds.top,
        bounds.right - bounds.left, bounds.bottom - bounds.top, SWP_SHOWWINDOW);
    SetForegroundWindow(alarmEditorWindow);
    SetFocus(alarmHourEdit);
}
