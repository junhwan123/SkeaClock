// Numeric IDs keep the menu icons after the main icon (ID 1) in Explorer.
#define SKEA_MENU_BELL 2
#define SKEA_MENU_MOVE 3
#define SKEA_MENU_VM 4
#define SKEA_MENU_CLOSE 5
#define SKEA_MENU_GLOBE 6
#define SKEA_MENU_GITHUB 7

#ifndef RC_INVOKED
#pragma once

HBITMAP CreateMenuIconBitmap(HICON icon, int width, int height) {
    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(screen, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HDC memory = bitmap ? CreateCompatibleDC(screen) : nullptr;
    if (memory) {
        HGDIOBJ previous = SelectObject(memory, bitmap);
        DrawIconEx(memory, 0, 0, icon, width, height, 0, nullptr, DI_NORMAL);
        SelectObject(memory, previous);
        DeleteDC(memory);
    }
    ReleaseDC(nullptr, screen);
    if (!memory && bitmap) {
        DeleteObject(bitmap);
        bitmap = nullptr;
    }
    return bitmap;
}

struct MenuIcons {
    UINT dpi;
    std::vector<HBITMAP> bitmaps;
    ~MenuIcons() { for (HBITMAP bitmap : bitmaps) DeleteObject(bitmap); }

    void SetLast(HMENU menu, WORD resource) {
        const int size = std::max(1, std::min(GetSystemMetricsForDpi(SM_CXMENUCHECK, dpi),
            GetSystemMetricsForDpi(SM_CYMENUCHECK, dpi)));
        HICON icon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(resource), IMAGE_ICON,
            size, size, 0));
        if (!icon) return;
        HBITMAP bitmap = CreateMenuIconBitmap(icon, size, size);
        DestroyIcon(icon);
        if (!bitmap) return;
        bitmaps.push_back(bitmap);

        // Match native menu text, including Windows high contrast colors.
        BITMAP data = {};
        GetObjectW(bitmap, sizeof(data), &data);
        auto pixels = static_cast<BYTE*>(data.bmBits);
        const COLORREF color = GetSysColor(COLOR_MENUTEXT);
        for (int i = 0; i < data.bmWidth * data.bmHeight; ++i) {
            const BYTE alpha = pixels[i * 4 + 3];
            pixels[i * 4] = GetBValue(color) * alpha / 255;
            pixels[i * 4 + 1] = GetGValue(color) * alpha / 255;
            pixels[i * 4 + 2] = GetRValue(color) * alpha / 255;
        }
        MENUITEMINFOW item = { sizeof(item) };
        item.fMask = MIIM_BITMAP;
        item.hbmpItem = bitmap;
        SetMenuItemInfoW(menu, GetMenuItemCount(menu) - 1, TRUE, &item);
    }
};
#endif
