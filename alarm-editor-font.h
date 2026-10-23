#pragma once

LOGFONTW alarmMenuFont = {};

LOGFONTW ReadAlarmMenuFont() {
    NONCLIENTMETRICSW metrics = { sizeof(metrics) };
    if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, alarmEditorDpi))
        return metrics.lfMenuFont;
    LOGFONTW font = {};
    GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(font), &font);
    return font;
}

HFONT CreateFittedAlarmFont(HDC dc, const wchar_t* value, int pixels, int width, int height) {
    if (!dc) return nullptr;
    LOGFONTW description = alarmMenuFont;
    description.lfHeight = -std::max(1, pixels);
    description.lfWidth = 0;
    HFONT font = CreateFontIndirectW(&description);
    if (!font) return nullptr;
    HGDIOBJ previous = SelectObject(dc, font);
    RECT size = {};
    if (value) DrawTextW(dc, value, -1, &size, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    else {
        // Time inputs must fit any pair of digits, not just the initial value.
        int widths[10] = {};
        TEXTMETRICW metrics = {};
        GetTextMetricsW(dc, &metrics);
        size.bottom = metrics.tmHeight;
        if (GetCharWidth32W(dc, L'0', L'9', widths))
            size.right = *std::max_element(std::begin(widths), std::end(widths)) * 2;
    }
    SelectObject(dc, previous);
    const float scale = std::min({ 1.0f, width / static_cast<float>(std::max(1L, size.right)),
        height / static_cast<float>(std::max(1L, size.bottom)) });
    if (scale < 1.0f) {
        DeleteObject(font);
        description.lfHeight = -std::max(1, static_cast<int>(pixels * scale));
        font = CreateFontIndirectW(&description);
    }
    return font;
}

void DrawAlarmLabel(HDC dc, const wchar_t* value, RECT bounds, int pixels, COLORREF color) {
    HFONT font = CreateFittedAlarmFont(dc, value, pixels, bounds.right - bounds.left, bounds.bottom - bounds.top);
    HGDIOBJ previous = SelectObject(dc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(dc, color);
    DrawTextW(dc, value, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, previous);
    if (font) DeleteObject(font);
}

void DrawAlarmEditorText(HDC dc, const RECT& client) {
    const int saved = SaveDC(dc);
    SetBkMode(dc, TRANSPARENT);
    const int actionSpace = MulDiv(82, alarmEditorDpi, 96);
    const int editWidth = MulDiv(48, alarmEditorDpi, 96);
    const int editGap = MulDiv(18, alarmEditorDpi, 96);
    const int groupLeft = std::max(MulDiv(14, alarmEditorDpi, 96),
        (static_cast<int>(client.right) - actionSpace - editWidth * 2 - editGap) / 2);
    const int colonX = groupLeft + editWidth + editGap / 2;
    const int colonWidth = MulDiv(8, alarmEditorDpi, 96);
    DrawAlarmLabel(dc, L":", { colonX - colonWidth, 0, colonX + colonWidth, client.bottom / 2 },
        MulDiv(22, alarmEditorDpi, 96), RGB(50, 63, 72));
    const float radius = std::min(14.0f * alarmEditorDpi / 96.0f, client.right / 7.0f * .4f);
    for (int day = 0; day < 7; ++day) {
        const PointF center = AlarmDayCenter(day, client.right, client.bottom);
        const RECT bounds = { day * client.right / 7, static_cast<LONG>(center.Y - radius),
            (day + 1) * client.right / 7, static_cast<LONG>(center.Y + radius) };
        const COLORREF color = day == 0 ? RGB(205, 62, 62) : day == 6 ? RGB(55, 105, 190) : RGB(50, 63, 72);
        DrawAlarmLabel(dc, UiText().alarmDays[day], bounds, MulDiv(14, alarmEditorDpi, 96), color);
    }
    if (saved) RestoreDC(dc, saved);
}
