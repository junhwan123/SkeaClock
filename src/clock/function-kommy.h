#pragma once

#include <cwchar>
#include "../animation/animation-kommy.h"
#include "../audio/timer-sound.h"
#include "../graphics/digital-segments.h"
#include "../graphics/kommy-dial.h"

namespace Kommy {
using KommyDial::FrameWidth;
using KommyDial::WhiteBorderRadius;
using KommyDial::FrameRadius;
using KommyDial::FrameDiameter;
std::unique_ptr<Bitmap> background;
std::unique_ptr<GraphicsPath> outerTimePanel, innerTimePanel;
int preparedPanelWidth = 0;
bool showTicks = true, showNumbers = true, showTime = false;
bool timePopupAbove = false;
bool showSecondHand = true;
int handSizePercent = 180;
ClockHand hand;

constexpr int MaximumTimerMinutes = 59;
constexpr ULONGLONG MaximumTimerSeconds = MaximumTimerMinutes * 60 + 59;

enum class TimeField { None, Minutes, Seconds };

struct TimeEditor {
    HWND window = nullptr, minutes = nullptr, seconds = nullptr;
    WNDPROC originalEditProc = nullptr;
    HFONT font = nullptr;
};

TimeEditor timeEditor;

ULONGLONG remainingSeconds = 0, countdownTick = 0, colonLitUntil = 0;
bool draggingHand = false;
float handDragPointerAngle = 0, handDragRemainder = 0;

float FullSecondHandScale() {
    return displayWidth / idle->GetWidth() * .15f * handSizePercent / 100.0f;
}
float SecondHandScale() { return FullSecondHandScale() * HandVisibility(); }
float SecondHandAngle() { return static_cast<float>(remainingSeconds % 60) * 6.0f; }

bool HandTouchEnabled() { return showSecondHand && handTargetScale > 0; }

RectF HandTouchBounds() {
    const float scale = FullSecondHandScale();
    const float length = std::hypot(hand.tip.X - hand.center.X, hand.tip.Y - hand.center.Y) * scale * 2.25f * .9f;
    const float width = hand.image->GetWidth() * scale * 1.2f;
    return RectF(-width / 2, -length, width, length);
}

float PointerAngle(PointF point) {
    return std::atan2(point.X - clockCenter.X, clockCenter.Y - point.Y) * 180 / Pi;
}

void AdvanceTimer(ULONGLONG now) {
    if (draggingHand || !remainingSeconds) return;
    const ULONGLONG elapsed = std::min((now - countdownTick) / 1000, remainingSeconds);
    remainingSeconds -= elapsed;
    countdownTick += elapsed * 1000;
    if (elapsed) {
        colonLitUntil = countdownTick + 500;
        PlayKommyDialTick();
        if (!remainingSeconds) {
            if (IsKommyCharacter()) StartTimerAlert(now);
            StartTimerAlarmSound();
        }
    }
}

void EndHandDrag() {
    if (!draggingHand) return;
    draggingHand = false;
    countdownTick = GetTickCount64();
    colonLitUntil = 0;
}

bool BeginHandDrag(PointF point) {
    if (!HandTouchEnabled()) return false;
    const float dx = point.X - clockCenter.X, dy = point.Y - clockCenter.Y;
    if (dx == 0 && dy == 0) return false;
    const float angle = SecondHandAngle() * Pi / 180;
    const float along = dx * std::sin(angle) - dy * std::cos(angle);
    const float across = dx * std::cos(angle) + dy * std::sin(angle);
    const RectF bounds = HandTouchBounds();
    if (along < 0 || along > -bounds.Y || std::abs(across) > bounds.Width / 2) return false;
    AdvanceTimer(GetTickCount64());
    handDragPointerAngle = PointerAngle(point);
    handDragRemainder = 0;
    draggingHand = true;
    return true;
}

void DragHand(PointF point) {
    if (point.X == clockCenter.X && point.Y == clockCenter.Y) return;
    const float angle = PointerAngle(point);
    handDragRemainder += std::remainder(angle - handDragPointerAngle, 360.0f);
    handDragPointerAngle = angle;
    if ((!remainingSeconds && handDragRemainder < 0) ||
        (remainingSeconds == MaximumTimerSeconds && handDragRemainder > 0)) handDragRemainder = 0;
    const int steps = static_cast<int>(std::lround(handDragRemainder / 6.0f));
    handDragRemainder -= steps * 6.0f;
    const ULONGLONG next = steps < 0
        ? remainingSeconds - std::min(remainingSeconds, static_cast<ULONGLONG>(-steps))
        : std::min(MaximumTimerSeconds, remainingSeconds + static_cast<ULONGLONG>(steps));
    if ((!next && steps < 0) || (next == MaximumTimerSeconds && steps > 0)) handDragRemainder = 0;
    if (next == remainingSeconds) return;
    remainingSeconds = next;
    PlayKommyDialTick();
}

void Release() {
    EndHandDrag();
    ReleaseAnimation();
    hand.image.reset();
    background.reset();
    outerTimePanel.reset();
    innerTimePanel.reset();
    preparedPanelWidth = 0;
}

bool Load(const std::wstring& directory) {
    const AnimationLayout config{ directory + L"locate data\\" KOMMY_LAYOUT_FILE };
    if (!LoadAnimation(directory, config)) return false;
    wchar_t file[MAX_PATH] = {};
    GetPrivateProfileStringW(L"hand", L"file", L"", file, MAX_PATH, config.path.c_str());
    if (!file[0]) return false;
    {
        Bitmap source((directory + file).c_str());
        if (source.GetLastStatus() != Ok) return false;
        hand.image = std::make_unique<Bitmap>(source.GetWidth(), source.GetHeight(), PixelFormat32bppPARGB);
        if (hand.image->GetLastStatus() != Ok) return false;
        Graphics graphics(hand.image.get());
        graphics.SetCompositingMode(CompositingModeSourceCopy);
        if (graphics.DrawImage(&source, Rect(0, 0, source.GetWidth(), source.GetHeight()),
            0, 0, source.GetWidth(), source.GetHeight(), UnitPixel) != Ok) return false;
    }
    if (!hand.image || hand.image->GetLastStatus() != Ok ||
        config.Number(L"hand", L"width") != hand.image->GetWidth() ||
        config.Number(L"hand", L"height") != hand.image->GetHeight()) return false;
    hand.center = PointF(config.Number(L"hand", L"centerX") + .5f, config.Number(L"hand", L"centerY") + .5f);
    hand.tip = PointF(config.Number(L"hand", L"tipX") + .5f, config.Number(L"hand", L"tipY") + .5f);
    for (const PointF point : { hand.center, hand.tip }) {
        if (!std::isfinite(point.X) || !std::isfinite(point.Y) || point.X < 0 || point.Y < 0 ||
            point.X >= hand.image->GetWidth() || point.Y >= hand.image->GetHeight()) return false;
    }
    const float dx = hand.tip.X - hand.center.X, dy = hand.tip.Y - hand.center.Y;
    if (dx == 0 && dy == 0) return false;
    hand.nativeAngle = std::atan2(dx, -dy) * 180 / Pi;
    if (!clockDial->InitializeMarkings(KommyDial::Numbers, KommyDial::Ticks)) return false;
    const int size = static_cast<int>(DialRadius * 2);
    background = std::make_unique<Bitmap>(size, size, PixelFormat32bppPARGB);
    if (background->GetLastStatus() != Ok) return false;
    Rect bounds(0, 0, size, size);
    BitmapData data = {};
    if (background->LockBits(&bounds, ImageLockModeWrite, PixelFormat32bppPARGB, &data) != Ok) return false;
    const Color pink = ClockCharacters[selectedCharacter].background;
    for (int y = 0; y < size; ++y) {
        auto* row = reinterpret_cast<ARGB*>(static_cast<BYTE*>(data.Scan0) + y * data.Stride);
        for (int x = 0; x < size; ++x) {
            float angle = std::atan2(x + .5f - DialRadius, DialRadius - y - .5f);
            if (angle < 0) angle += 2 * Pi;
            const float white = angle / (2 * Pi);
            row[x] = Color(255,
                static_cast<BYTE>(pink.GetR() + (255 - pink.GetR()) * white),
                static_cast<BYTE>(pink.GetG() + (255 - pink.GetG()) * white),
                static_cast<BYTE>(pink.GetB() + (255 - pink.GetB()) * white)).GetValue();
        }
    }
    return background->UnlockBits(&data) == Ok;
}

void Draw(Graphics& graphics, bool drawBackground, bool drawTicks, bool drawNumbers) {
    auto& dial = *clockDial;
    if (drawBackground) {
        const float left = clockCenter.X - FrameRadius, top = clockCenter.Y - FrameRadius;
        SolidBrush frame(ClockCharacters[selectedCharacter].background), border(Color(255, 255, 255, 255));
        const GraphicsState frameState = graphics.Save();
        graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
        if (showTime) {
            graphics.SetSmoothingMode(SmoothingModeNone);
            graphics.FillRectangle(&frame, left, timePopupAbove ? 0.0f : clockCenter.Y, FrameDiameter, clockCenter.Y);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        }
        graphics.FillEllipse(&frame, left, top, FrameDiameter, FrameDiameter);
        graphics.Restore(frameState);
        graphics.FillEllipse(&border, left + FrameWidth, top + FrameWidth, WhiteBorderRadius * 2, WhiteBorderRadius * 2);
        const GraphicsState state = graphics.Save();
        graphics.TranslateTransform(clockCenter.X, clockCenter.Y);
        graphics.RotateTransform(SecondHandAngle());
        graphics.TranslateTransform(-clockCenter.X, -clockCenter.Y);
        TextureBrush brush(background.get(), WrapModeClamp);
        brush.TranslateTransform(clockCenter.X - DialRadius, clockCenter.Y - DialRadius);
        graphics.FillPath(&brush, dial.clip.get());
        Pen seam(ClockCharacters[selectedCharacter].background, 1.0f);
        graphics.DrawLine(&seam, clockCenter, PointF(clockCenter.X, clockCenter.Y - DialRadius));
        graphics.Restore(state);
    }
    const GraphicsState clipState = ClipToDial(graphics);
    graphics.SetInterpolationMode(InterpolationModeBilinear);
    DrawCharacter(graphics);
    graphics.Restore(clipState);
    for (int i = 0; i < 60; ++i) {
        if (drawTicks) {
            graphics.DrawLine(&dial.minorOutline, dial.tickStart[i], dial.tickEnd[i]);
            graphics.DrawLine(&dial.minorTick, dial.tickStart[i], dial.tickEnd[i]);
        }
    }
    if (drawNumbers) {
        for (auto& number : dial.numbers) {
            graphics.DrawPath(&dial.numberOutline, number.get());
            graphics.FillPath(&dial.ink, number.get());
        }
    }
    const float handScale = SecondHandScale();
    if (showSecondHand && handScale > 0) {
        const GraphicsState state = graphics.Save();
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        Matrix transform;
        transform.Translate(clockCenter.X, clockCenter.Y);
        transform.Rotate(SecondHandAngle() - hand.nativeAngle);
        transform.Scale(handScale, handScale);
        transform.Translate(-hand.center.X, -hand.center.Y);
        graphics.MultiplyTransform(&transform);
        graphics.DrawImage(hand.image.get(), RectF(0, 0, static_cast<float>(hand.image->GetWidth()),
            static_cast<float>(hand.image->GetHeight())), 0, 0, static_cast<float>(hand.image->GetWidth()),
            static_cast<float>(hand.image->GetHeight()), UnitPixel);
        graphics.Restore(state);
    }
    Pen minuteHand(Color(255, 255, 0, 0), 2.0f);
    const float minuteAngle = static_cast<float>(remainingSeconds / 60 % 60) * Pi / 30;
    const PointF minuteDirection(std::sin(minuteAngle), -std::cos(minuteAngle));
    graphics.DrawLine(&minuteHand,
        PointF(clockCenter.X + minuteDirection.X * DialRadius, clockCenter.Y + minuteDirection.Y * DialRadius),
        PointF(clockCenter.X + minuteDirection.X * WhiteBorderRadius,
            clockCenter.Y + minuteDirection.Y * WhiteBorderRadius));
}

constexpr int TimeDigitCells = 5;
constexpr float TimeDigitCellWidth = 24;
constexpr float TimeDigitHeight = 32 * DigitalSegments::HeightScale;

RectF DigitalTimeBounds(const RectF& bounds) {
    const float width = TimeDigitCells * TimeDigitCellWidth;
    const float scale = std::min(bounds.Width / width, bounds.Height / TimeDigitHeight) * .72f;
    return RectF(bounds.X + (bounds.Width - width * scale) / 2,
        bounds.Y + (bounds.Height - TimeDigitHeight * scale) / 2, width * scale, TimeDigitHeight * scale);
}

void DrawDigitalTime(Graphics& graphics, const RectF& bounds, const wchar_t* text, bool colonLit) {
    GraphicsPath digits(FillModeWinding);
    for (int cell = 0; cell < TimeDigitCells; ++cell) {
        const float left = cell * TimeDigitCellWidth + 3;
        if (text[cell] == L':') {
            if (colonLit) {
                digits.AddRectangle(RectF(left + 7, 8, 4, 4));
                digits.AddRectangle(RectF(left + 7, 20, 4, 4));
            }
            continue;
        }
        const UINT mask = DigitalSegments::NumberMasks[text[cell] - L'0'];
        for (int segment = 0; segment < 7; ++segment) {
            if (!(mask & (1u << segment))) continue;
            const auto& part = DigitalSegments::Parts[segment];
            PointF polygon[6];
            for (int vertex = 0; vertex < part.count; ++vertex)
                polygon[vertex] = PointF(left + part.points[vertex].X, part.points[vertex].Y);
            digits.AddPolygon(polygon, part.count);
        }
    }
    const float scale = bounds.Height / TimeDigitHeight;
    Matrix placement(scale, 0, -DigitalSegments::Slant * scale, DigitalSegments::HeightScale * scale,
        bounds.X + 16 * DigitalSegments::Slant * scale, bounds.Y);
    digits.Transform(&placement);
    SolidBrush ink(Color(255, 50, 63, 72));
    graphics.FillPath(&ink, &digits);
}

void PrepareTimePanelPath(GraphicsPath& shape, const RectF& bounds, float corner) {
    shape.AddArc(bounds.X, bounds.Y, corner, corner, 180.0f, 90.0f);
    shape.AddArc(bounds.GetRight() - corner, bounds.Y, corner, corner, 270.0f, 90.0f);
    shape.AddArc(bounds.GetRight() - corner, bounds.GetBottom() - corner, corner, corner, 0.0f, 90.0f);
    shape.AddArc(bounds.X, bounds.GetBottom() - corner, corner, corner, 90.0f, 90.0f);
    shape.CloseFigure();
}

struct TimePanelSize {
    float width, inset, overlap;
    int height;
};

TimePanelSize GetTimePanelSize(int clockWidth) {
    const float width = FrameDiameter * clockWidth / artworkWidth;
    const float inset = FrameWidth * clockWidth / artworkWidth;
    return { width, inset, (clockWidth - width) / 2 + inset, std::max(1, static_cast<int>(width / KommyDial::PanelHeightDivisor)) };
}

RectF TimePanelBounds(const RECT& clock, const TimePanelSize& panel) {
    const float height = static_cast<float>(panel.height);
    const float left = clock.left + (clock.right - clock.left - panel.width) / 2;
    const float top = timePopupAbove ? clock.top + panel.overlap - height : clock.bottom - panel.overlap;
    return RectF(left, top, panel.width, height);
}

RectF TimePanelDigits(const RECT& clock) {
    const auto panel = GetTimePanelSize(clock.right - clock.left);
    RectF bounds = TimePanelBounds(clock, panel);
    bounds.Inflate(-panel.inset, -panel.inset);
    return DigitalTimeBounds(bounds);
}

RectF TimePartBounds(const RectF& digits, TimeField field) {
    const float cell = digits.Width / TimeDigitCells;
    const float padding = cell / 6;
    const float start = field == TimeField::Minutes ? digits.X : digits.X + 3 * cell;
    return RectF(start - padding, digits.Y - padding, cell * 2 + padding * 2, digits.Height + padding * 2);
}

TimeField TimeFieldAt(const RECT& clock, POINT point) {
    if (!IsKommyCharacter() || !showTime) return TimeField::None;
    const RectF digits = TimePanelDigits(clock);
    const float x = static_cast<float>(point.x), y = static_cast<float>(point.y);
    if (TimePartBounds(digits, TimeField::Minutes).Contains(x, y)) return TimeField::Minutes;
    if (TimePartBounds(digits, TimeField::Seconds).Contains(x, y)) return TimeField::Seconds;
    return TimeField::None;
}

void DrawTimePanel(Graphics& graphics, const RECT& clock) {
    if (!IsKommyCharacter() || !showTime) return;
    const int width = clock.right - clock.left;
    const TimePanelSize panel = GetTimePanelSize(width);
    const RectF bounds = TimePanelBounds(clock, panel);
    const float w = bounds.Width, h = bounds.Height;
    const float corner = h * KommyDial::PanelCornerRatio;
    const float inset = panel.inset;
    SolidBrush fill(ClockCharacters[1].background), white(Color(255, 255, 255, 255));
    if (preparedPanelWidth != width) {
        outerTimePanel = std::make_unique<GraphicsPath>();
        innerTimePanel = std::make_unique<GraphicsPath>();
        PrepareTimePanelPath(*outerTimePanel, RectF(0, 0, w, h), corner);
        PrepareTimePanelPath(*innerTimePanel, RectF(inset, inset, w - 2 * inset, h - 2 * inset), corner - 2 * inset);
        preparedPanelWidth = width;
    }
    const GraphicsState state = graphics.Save();
    graphics.TranslateTransform(bounds.X, bounds.Y);
    graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
    graphics.FillPath(&fill, outerTimePanel.get());
    const float joinY = timePopupAbove ? h - corner / 2 : 0;
    graphics.SetSmoothingMode(SmoothingModeNone);
    graphics.FillRectangle(&fill, 0.0f, joinY, w, corner / 2);
    graphics.Restore(state);
    const GraphicsState innerState = graphics.Save();
    graphics.TranslateTransform(bounds.X, bounds.Y);
    graphics.FillPath(&white, innerTimePanel.get());
    graphics.Restore(innerState);
    if (timeEditor.window) return;
    RectF timeBounds = bounds;
    timeBounds.Inflate(-inset, -inset);
    const bool colonLit = !draggingHand && remainingSeconds && GetTickCount64() < colonLitUntil;
    wchar_t text[TimeDigitCells + 1] = {};
    swprintf_s(text, L"%02llu:%02llu", remainingSeconds / 60, remainingSeconds % 60);
    DrawDigitalTime(graphics, DigitalTimeBounds(timeBounds), text, colonLit);
}

void SelectTimePart(HWND field) {
    SetFocus(field);
    SendMessageW(field, EM_SETSEL, 0, -1);
}

bool ReadTimePart(HWND field, int& value) {
    wchar_t text[4] = {};
    const int length = GetWindowTextW(field, text, _countof(text));
    if (length < 1 || length > 2) return false;
    for (int i = 0; i < length; ++i)
        if (text[i] < L'0' || text[i] > L'9') return false;
    value = text[0] - L'0';
    if (length == 2) value = value * 10 + text[1] - L'0';
    return true;
}

void CloseTimeEditor(bool confirm = false) {
    if (!timeEditor.window) return;
    const bool minutesChanged = SendMessageW(timeEditor.minutes, EM_GETMODIFY, 0, 0) != FALSE;
    const bool secondsChanged = SendMessageW(timeEditor.seconds, EM_GETMODIFY, 0, 0) != FALSE;
    int minutes = static_cast<int>(remainingSeconds / 60), seconds = static_cast<int>(remainingSeconds % 60);
    HWND invalid = nullptr;
    if (minutesChanged && !ReadTimePart(timeEditor.minutes, minutes)) invalid = timeEditor.minutes;
    else if (secondsChanged && !ReadTimePart(timeEditor.seconds, seconds)) invalid = timeEditor.seconds;
    if (invalid && confirm) {
        MessageBeep(MB_ICONWARNING);
        SelectTimePart(invalid);
        return;
    }
    if (!invalid && (minutesChanged || secondsChanged)) {
        const auto total = static_cast<ULONGLONG>(std::min(minutes, MaximumTimerMinutes) * 60 + seconds);
        remainingSeconds = std::min(MaximumTimerSeconds, total);
        countdownTick = GetTickCount64();
        colonLitUntil = 0;
    }
    DestroyWindow(timeEditor.window);
}

LRESULT CALLBACK TimePartProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CHAR) {
        const bool editingMinutes = window == timeEditor.minutes;
        if (wParam == VK_TAB || wParam == VK_RETURN || wParam == VK_SPACE) {
            if (editingMinutes) SelectTimePart(timeEditor.seconds);
            else CloseTimeEditor(true);
            return 0;
        }
        const LRESULT result = CallWindowProcW(timeEditor.originalEditProc, window, message, wParam, lParam);
        if (editingMinutes && wParam >= L'0' && wParam <= L'9' && GetWindowTextLengthW(window) == 2)
            SelectTimePart(timeEditor.seconds);
        return result;
    }
    if (message == WM_CONTEXTMENU)
        return SendMessageW(timeEditor.window, WM_CONTEXTMENU, wParam, lParam);
    return CallWindowProcW(timeEditor.originalEditProc, window, message, wParam, lParam);
}

LRESULT CALLBACK TimeEditorProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wParam), RGB(50, 63, 72));
        SetBkColor(reinterpret_cast<HDC>(wParam), RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) PostMessageW(window, WM_CLOSE, 0, 0);
        break;
    case WM_CONTEXTMENU: {
        const HWND owner = GetWindow(window, GW_OWNER);
        PostMessageW(owner, WM_CONTEXTMENU, reinterpret_cast<WPARAM>(owner), lParam);
        return 0;
    }
    case WM_CLOSE:
        CloseTimeEditor();
        return 0;
    case WM_DESTROY:
        if (timeEditor.originalEditProc) {
            SetWindowLongPtrW(timeEditor.minutes, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(timeEditor.originalEditProc));
            SetWindowLongPtrW(timeEditor.seconds, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(timeEditor.originalEditProc));
        }
        if (timeEditor.font) DeleteObject(timeEditor.font);
        timeEditor = {};
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void OpenTimeEditor(HWND owner, const RECT& clock, TimeField field) {
    if (timeEditor.window) return;
    constexpr wchar_t className[] = L"KommyTimeEditorWindow";
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    static const ATOM registered = [&] {
        WNDCLASSW editorClass = {};
        editorClass.hInstance = instance;
        editorClass.lpfnWndProc = TimeEditorProc;
        editorClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        editorClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
        editorClass.lpszClassName = className;
        return RegisterClassW(&editorClass);
    }();
    if (!registered) return;

    const RectF digits = TimePanelDigits(clock);
    const RectF minutes = TimePartBounds(digits, TimeField::Minutes), seconds = TimePartBounds(digits, TimeField::Seconds);
    const int left = static_cast<int>(std::floor(minutes.X)), top = static_cast<int>(std::floor(minutes.Y));
    const int minuteWidth = static_cast<int>(std::ceil(minutes.GetRight())) - left;
    const int secondLeft = static_cast<int>(std::floor(seconds.X)) - left;
    const int width = static_cast<int>(std::ceil(seconds.GetRight())) - left;
    const int height = static_cast<int>(std::ceil(minutes.GetBottom())) - top;
    POINT origin = { left, top };
    if (!ClientToScreen(owner, &origin)) return;
    // The owned window lets the edit controls paint above the layered clock.
    const HWND popup = CreateWindowExW(WS_EX_TOOLWINDOW, className, nullptr, WS_POPUP | WS_CLIPCHILDREN,
        origin.x, origin.y, width, height, owner, nullptr, instance, nullptr);
    if (!popup) return;
    timeEditor.window = popup;
    const auto createPart = [&](int value, int x, int partWidth) {
        wchar_t text[3] = {};
        swprintf_s(text, L"%02d", value);
        const HWND edit = CreateWindowExW(0, L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER,
            x, 0, partWidth, height, popup, nullptr, instance, nullptr);
        if (edit) SendMessageW(edit, EM_SETLIMITTEXT, 2, 0);
        return edit;
    };
    timeEditor.minutes = createPart(static_cast<int>(remainingSeconds / 60), 0, minuteWidth);
    timeEditor.seconds = createPart(static_cast<int>(remainingSeconds % 60), secondLeft, width - secondLeft);
    const HWND separator = CreateWindowExW(0, L"STATIC", L":", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        minuteWidth, 0, secondLeft - minuteWidth, height, popup, nullptr, instance, nullptr);
    if (!timeEditor.minutes || !timeEditor.seconds || !separator) {
        DestroyWindow(popup);
        return;
    }
    timeEditor.originalEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(timeEditor.minutes, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(TimePartProc)));
    if (!timeEditor.originalEditProc ||
        !SetWindowLongPtrW(timeEditor.seconds, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(TimePartProc))) {
        DestroyWindow(popup);
        return;
    }
    timeEditor.font = CreateFontW(-std::max(1, height - 4), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    if (timeEditor.font)
        for (HWND control : { timeEditor.minutes, timeEditor.seconds, separator })
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(timeEditor.font), FALSE);
    ShowWindow(popup, SW_SHOW);
    SetForegroundWindow(popup);
    SelectTimePart(field == TimeField::Minutes ? timeEditor.minutes : timeEditor.seconds);
}
}
