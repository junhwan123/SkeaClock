#pragma once

#include "../clock/function-kommy.h"
#include "window-layout-settings.h"

int sizePercent = 100;
std::wstring selectedDisplay, activeDisplay;

struct DisplayInfo {
    MONITORINFOEXW info;
};

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

int clockTopInset = 0, clockBottomInset = 0;

bool GetClockClientRect(HWND window, RECT& bounds) {
    if (!GetClientRect(window, &bounds)) return false;
    bounds.top += clockTopInset;
    bounds.bottom -= clockBottomInset;
    return true;
}

bool GetClockWindowRect(HWND window, RECT& bounds) {
    if (!GetWindowRect(window, &bounds)) return false;
    bounds.top += clockTopInset;
    bounds.bottom -= clockBottomInset;
    return true;
}

void SetClockWindowRect(HWND window, RECT bounds) {
    clockTopInset = clockBottomInset = 0;
    Kommy::timePopupAbove = false;
    if (IsKommyCharacter() && Kommy::showTime) {
        const Kommy::TimePanelSize panel = Kommy::GetTimePanelSize(bounds.right - bounds.left);
        const int extra = static_cast<int>(std::ceil(panel.height - panel.overlap));
        MONITORINFO monitor = { sizeof(monitor) };
        const bool haveWork = GetMonitorInfoW(MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST), &monitor) != FALSE;
        Kommy::timePopupAbove = haveWork && bounds.bottom + extra > monitor.rcWork.bottom;
        if (Kommy::timePopupAbove) {
            clockTopInset = extra;
            bounds.top -= extra;
        } else {
            clockBottomInset = extra;
            bounds.bottom += extra;
        }
        if (haveWork) {
            const int height = bounds.bottom - bounds.top;
            const int top = std::clamp(bounds.top, monitor.rcWork.top,
                std::max(monitor.rcWork.top, monitor.rcWork.bottom - height));
            OffsetRect(&bounds, 0, top - bounds.top);
        }
    }
    SetWindowPos(window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left,
        bounds.bottom - bounds.top, SWP_NOACTIVATE | SWP_NOZORDER);
}

constexpr float LayoutInset = 5.0f;
constexpr float LayoutCornerHitRadius = 20.0f;
constexpr float LayoutMoveHitRadius = 26.0f;
constexpr float LayoutCloseHitRadius = 18.0f;

enum class LayoutHandle { None, Move, TopLeft, TopRight, BottomLeft, BottomRight, Close };

bool layoutEditing = false;
LayoutHandle layoutDrag = LayoutHandle::None;
RECT layoutDragStartRect = {};
POINT layoutDragStartCursor = {};
UINT layoutDragDpi = 96;

SIZE LayoutSizeForDpi(int percent, UINT dpi) {
    return { std::max(1, MulDiv(artworkWidth, static_cast<int>(dpi) * percent, 9600)),
        std::max(1, MulDiv(artworkHeight, static_cast<int>(dpi) * percent, 9600)) };
}

void ClampLayoutRect(RECT& bounds, const RECT& work) {
    const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    bounds.left = std::clamp(bounds.left, work.left, std::max(work.left, work.right - width));
    bounds.top = std::clamp(bounds.top, work.top, std::max(work.top, work.bottom - height));
    bounds.right = bounds.left + width;
    bounds.bottom = bounds.top + height;
}

bool ActiveWorkArea(RECT& work) {
    for (const auto& display : GetDisplays()) {
        if (_wcsicmp(display.info.szDevice, activeDisplay.c_str()) == 0) {
            work = display.info.rcWork;
            return true;
        }
    }
    return false;
}

void SaveCurrentLayout(HWND window) {
    RECT bounds;
    if (!activeDisplay.empty() && GetClockWindowRect(window, bounds))
        WriteDisplayLayout(activeDisplay, bounds, sizePercent);
}

void PositionClock(HWND window) {
    const auto displays = GetDisplays();
    const DisplayInfo* display = ChooseDisplay(displays);
    if (!display) return;
    activeDisplay = display->info.szDevice;
    RECT bounds = {};
    int savedPercent = 100;
    const bool saved = ReadDisplayLayout(activeDisplay, bounds, savedPercent);
    sizePercent = saved ? savedPercent : 100;
    const UINT dpi = GetDpiForWindow(window);
    const SIZE size = LayoutSizeForDpi(sizePercent, dpi);
    if (!saved) {
        const int margin = MulDiv(16, dpi, 96);
        bounds.left = display->info.rcWork.right - size.cx - margin;
        bounds.top = display->info.rcWork.bottom - size.cy - margin;
    }
    bounds.right = bounds.left + size.cx;
    bounds.bottom = bounds.top + size.cy;
    ClampLayoutRect(bounds, display->info.rcWork);
    SetClockWindowRect(window, bounds);
}

PointF LayoutCloseCenter(int width) { return PointF(width - 34.0f, 34.0f); }

bool InLayoutCircle(float x, float y, PointF center, float radius) {
    return std::hypot(x - center.X, y - center.Y) <= radius;
}

LayoutHandle LayoutHitAt(HWND window, float x, float y) {
    RECT bounds;
    if (!GetClockClientRect(window, bounds)) return LayoutHandle::None;
    const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    y -= bounds.top;
    if (InLayoutCircle(x, y, LayoutCloseCenter(width), LayoutCloseHitRadius)) return LayoutHandle::Close;
    const struct { LayoutHandle handle; PointF point; } corners[] = {
        { LayoutHandle::TopLeft, PointF(LayoutInset, LayoutInset) },
        { LayoutHandle::TopRight, PointF(width - LayoutInset, LayoutInset) },
        { LayoutHandle::BottomLeft, PointF(LayoutInset, height - LayoutInset) },
        { LayoutHandle::BottomRight, PointF(width - LayoutInset, height - LayoutInset) }
    };
    for (const auto& corner : corners)
        if (std::abs(x - corner.point.X) <= LayoutCornerHitRadius &&
            std::abs(y - corner.point.Y) <= LayoutCornerHitRadius) return corner.handle;
    if (InLayoutCircle(x, y, PointF(width * .5f, height * .5f), LayoutMoveHitRadius)) return LayoutHandle::Move;
    return LayoutHandle::None;
}

HCURSOR LayoutCursor(LayoutHandle handle) {
    if (handle == LayoutHandle::Move) return LoadCursorW(nullptr, IDC_SIZEALL);
    if (handle == LayoutHandle::TopLeft || handle == LayoutHandle::BottomRight)
        return LoadCursorW(nullptr, IDC_SIZENWSE);
    if (handle == LayoutHandle::TopRight || handle == LayoutHandle::BottomLeft)
        return LoadCursorW(nullptr, IDC_SIZENESW);
    if (handle == LayoutHandle::Close) return LoadCursorW(nullptr, IDC_HAND);
    return LoadCursorW(nullptr, IDC_ARROW);
}

struct LayoutResizeResult { RECT bounds; int percent; };

LayoutResizeResult CalculateLayoutResize(const RECT& start, LayoutHandle handle, POINT cursor, UINT dpi) {
    const SIZE base = LayoutSizeForDpi(100, dpi);
    POINT fixed = {};
    float desiredWidth = 0, desiredHeight = 0;
    if (handle == LayoutHandle::TopLeft) {
        fixed = { start.right, start.bottom };
        desiredWidth = static_cast<float>(fixed.x - cursor.x);
        desiredHeight = static_cast<float>(fixed.y - cursor.y);
    } else if (handle == LayoutHandle::TopRight) {
        fixed = { start.left, start.bottom };
        desiredWidth = static_cast<float>(cursor.x - fixed.x);
        desiredHeight = static_cast<float>(fixed.y - cursor.y);
    } else if (handle == LayoutHandle::BottomLeft) {
        fixed = { start.right, start.top };
        desiredWidth = static_cast<float>(fixed.x - cursor.x);
        desiredHeight = static_cast<float>(cursor.y - fixed.y);
    } else {
        fixed = { start.left, start.top };
        desiredWidth = static_cast<float>(cursor.x - fixed.x);
        desiredHeight = static_cast<float>(cursor.y - fixed.y);
    }
    const float projectedScale = (desiredWidth * base.cx + desiredHeight * base.cy) /
        static_cast<float>(base.cx * base.cx + base.cy * base.cy);
    const int percent = std::clamp(static_cast<int>(std::lround(projectedScale * 100)),
        MinimumLayoutPercent, MaximumLayoutPercent);
    const SIZE size = LayoutSizeForDpi(percent, dpi);
    RECT bounds = {};
    if (handle == LayoutHandle::TopLeft) bounds = { fixed.x - size.cx, fixed.y - size.cy, fixed.x, fixed.y };
    else if (handle == LayoutHandle::TopRight) bounds = { fixed.x, fixed.y - size.cy, fixed.x + size.cx, fixed.y };
    else if (handle == LayoutHandle::BottomLeft) bounds = { fixed.x - size.cx, fixed.y, fixed.x, fixed.y + size.cy };
    else bounds = { fixed.x, fixed.y, fixed.x + size.cx, fixed.y + size.cy };
    return { bounds, percent };
}

void BeginLayoutDrag(HWND window, LayoutHandle handle) {
    layoutDrag = handle;
    GetClockWindowRect(window, layoutDragStartRect);
    GetCursorPos(&layoutDragStartCursor);
    layoutDragDpi = GetDpiForWindow(window);
    SetCapture(window);
}

void UpdateLayoutDrag(HWND window) {
    if (layoutDrag == LayoutHandle::None) return;
    POINT cursor;
    GetCursorPos(&cursor);
    RECT bounds = layoutDragStartRect;
    if (layoutDrag == LayoutHandle::Move) {
        const int dx = cursor.x - layoutDragStartCursor.x, dy = cursor.y - layoutDragStartCursor.y;
        OffsetRect(&bounds, dx, dy);
    } else {
        const LayoutResizeResult resized = CalculateLayoutResize(layoutDragStartRect, layoutDrag, cursor, layoutDragDpi);
        bounds = resized.bounds;
        sizePercent = resized.percent;
    }
    RECT work;
    if (ActiveWorkArea(work)) ClampLayoutRect(bounds, work);
    SetClockWindowRect(window, bounds);
}

void DrawLayoutEditor(Graphics& graphics, int width, int height) {
    if (!layoutEditing) return;
    SolidBrush invisibleHit(Color(1, 255, 255, 255));
    SolidBrush handleFill(Color(230, 105, 110, 115));
    SolidBrush cornerFill(Color(240, 245, 245, 245));
    Pen border(Color(230, 105, 110, 115), 2.0f);
    Pen light(Color(245, 255, 255, 255), 2.0f);
    const RectF outline(LayoutInset, LayoutInset, width - LayoutInset * 2, height - LayoutInset * 2);
    graphics.DrawRectangle(&border, outline);
    const PointF corners[] = { PointF(LayoutInset, LayoutInset), PointF(width - LayoutInset, LayoutInset),
        PointF(LayoutInset, height - LayoutInset), PointF(width - LayoutInset, height - LayoutInset) };
    for (const PointF& corner : corners) {
        graphics.FillRectangle(&invisibleHit, corner.X - LayoutCornerHitRadius,
            corner.Y - LayoutCornerHitRadius, LayoutCornerHitRadius * 2, LayoutCornerHitRadius * 2);
        graphics.FillRectangle(&cornerFill, corner.X - 5.0f, corner.Y - 5.0f, 10.0f, 10.0f);
        graphics.DrawRectangle(&border, corner.X - 5.0f, corner.Y - 5.0f, 10.0f, 10.0f);
    }
    const PointF move(width * .5f, height * .5f);
    graphics.FillEllipse(&invisibleHit, move.X - LayoutMoveHitRadius, move.Y - LayoutMoveHitRadius,
        LayoutMoveHitRadius * 2, LayoutMoveHitRadius * 2);
    graphics.FillEllipse(&handleFill, move.X - 11.0f, move.Y - 11.0f, 22.0f, 22.0f);
    graphics.DrawEllipse(&light, move.X - 11.0f, move.Y - 11.0f, 22.0f, 22.0f);
    graphics.DrawLine(&light, move.X - 16, move.Y, move.X + 16, move.Y);
    graphics.DrawLine(&light, move.X, move.Y - 16, move.X, move.Y + 16);
    const PointF close = LayoutCloseCenter(width);
    graphics.FillEllipse(&invisibleHit, close.X - LayoutCloseHitRadius, close.Y - LayoutCloseHitRadius,
        LayoutCloseHitRadius * 2, LayoutCloseHitRadius * 2);
    graphics.FillEllipse(&handleFill, close.X - 12.0f, close.Y - 12.0f, 24.0f, 24.0f);
    graphics.DrawLine(&light, close.X - 5, close.Y - 5, close.X + 5, close.Y + 5);
    graphics.DrawLine(&light, close.X + 5, close.Y - 5, close.X - 5, close.Y + 5);
}
