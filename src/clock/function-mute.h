#pragma once

#include <random>
#include "../animation/animation-mute.h"
#include "../graphics/digital-segments.h"

namespace Mute {
bool showBackground = false;

constexpr float DigitalStrokeWidth = 0.2f;
bool hologramEnabled = true;
bool use24Hour = true;
bool showInactiveSegments = false;

struct DigitalDisplayPaths {
    GraphicsPath lit{ FillModeWinding };
    GraphicsPath unlit{ FillModeWinding };
    GraphicsPath indicators{ FillModeWinding };
};

void DrawHologramOverlay(Graphics& graphics, const GraphicsPath& shape) {
    constexpr float period = DigitalStrokeWidth * 6;
    const Color clear(0, 0x5B, 0xCD, 0xBE);
    const Color light(255, 0x5B, 0xCD, 0xBE);
    const Color boundary(128, 0x5B, 0xCD, 0xBE);
    const Color colors[] = { boundary, clear, clear, light, light, boundary };
    const REAL positions[] = { 0, 1.0f / 24, 19.0f / 24, 21.0f / 24, 23.0f / 24, 1 };
    LinearGradientBrush pattern(PointF(0, 0), PointF(0, period), clear, clear);
    pattern.SetInterpolationColors(colors, positions, 6);
    pattern.SetWrapMode(WrapModeTile);
    pattern.TranslateTransform(0, static_cast<float>(GetTickCount64() % 6000) * period / 6000.0f, MatrixOrderAppend);
    graphics.FillPath(&pattern, &shape);
    Pen edge(light, DigitalStrokeWidth);
    edge.SetAlignment(PenAlignmentInset);
    graphics.DrawPath(&edge, &shape);
}

void DrawDigitalFace(Graphics& graphics, DigitalDisplayPaths& display) {
    if (display.unlit.GetPointCount()) {
        SolidBrush inactive(Color(72, 0x0A, 0x3E, 0x39));
        graphics.FillPath(&inactive, &display.unlit);
    }
    SolidBrush ink(Color(255, 0x14, 0x7D, 0x73));
    graphics.FillPath(&ink, &display.lit);
    if (hologramEnabled) DrawHologramOverlay(graphics, display.lit);
    if (display.indicators.GetPointCount()) {
        SolidBrush indicator(Color(255, 0x5B, 0xCD, 0xBE));
        graphics.FillPath(&indicator, &display.indicators);
    }
}

struct DigitalGlitchStripe {
    float top, height, offset;
};

void DrawDigitalGlitch(Graphics& graphics, DigitalDisplayPaths& display, RectF bounds,
    PointF center, float radius, const std::vector<DigitalGlitchStripe>& stripes) {
    Matrix transform;
    graphics.GetTransform(&transform);
    REAL elements[6];
    transform.GetElements(elements);
    const float scaleX = elements[0], scaleY = elements[3];
    float padding = 3;
    for (const auto& stripe : stripes) padding = std::max(padding, std::abs(stripe.offset) + .5f);
    const int left = static_cast<int>(std::floor((bounds.X - padding) * scaleX));
    const int top = static_cast<int>(std::floor((bounds.Y - 1) * scaleY));
    const int width = static_cast<int>(std::ceil((bounds.GetRight() + padding) * scaleX)) - left;
    const int height = static_cast<int>(std::ceil((bounds.GetBottom() + 1) * scaleY)) - top;
    std::vector<ARGB> pixels(static_cast<size_t>(width) * height);
    Bitmap image(width, height, width * 4, PixelFormat32bppPARGB, reinterpret_cast<BYTE*>(pixels.data()));
    if (image.GetLastStatus() != Ok) {
        DrawDigitalFace(graphics, display);
        return;
    }
    {
        Graphics layer(&image);
        layer.SetSmoothingMode(SmoothingModeAntiAlias);
        Matrix placement(scaleX, 0, 0, scaleY, static_cast<float>(-left), static_cast<float>(-top));
        layer.SetTransform(&placement);
        DrawDigitalFace(layer, display);
        layer.Flush(FlushIntentionSync);
    }
    const auto original = pixels;
    size_t stripeIndex = 0;
    for (int y = 0; y < height; ++y) {
        const float worldY = (top + y + .5f) / scaleY;
        while (stripeIndex < stripes.size() && worldY >= stripes[stripeIndex].top + stripes[stripeIndex].height)
            ++stripeIndex;
        if (stripeIndex == stripes.size()) break;
        const auto& stripe = stripes[stripeIndex];
        const float dy = worldY - center.Y;
        const float chordSquared = radius * radius - dy * dy;
        if (worldY < stripe.top || chordSquared <= 0) continue;
        const float chord = std::sqrt(chordSquared);
        const int first = std::max(0, static_cast<int>(std::ceil((center.X - chord) * scaleX - left - .5f)));
        const int last = std::min(width, static_cast<int>(std::ceil((center.X + chord) * scaleX - left - .5f)));
        const int shift = static_cast<int>(std::round(stripe.offset * scaleX));
        const size_t row = static_cast<size_t>(y) * width;
        for (int x = first; x < last; ++x) {
            const int sourceX = x - shift;
            pixels[row + x] = sourceX >= 0 && sourceX < width ? original[row + sourceX] : 0;
        }
    }
    graphics.DrawImage(&image, RectF(left / scaleX, top / scaleY, width / scaleX, height / scaleY),
        0, 0, static_cast<float>(width), static_cast<float>(height), UnitPixel);
}

void DrawDigitalDisplay(Graphics& graphics, DigitalDisplayPaths& display) {
    static std::mt19937 generator(std::random_device{}());
    static ULONGLONG nextGlitch = 0, glitchStart = 0;
    static UINT duration = 0;
    static PointF center;
    static float radius = 0;
    static std::vector<DigitalGlitchStripe> stripes;
    if (!hologramEnabled) {
        digitalGlitchRequested = false;
        nextGlitch = 0;
        stripes.clear();
        DrawDigitalFace(graphics, display);
        return;
    }
    auto random = [&](UINT low, UINT high) {
        return std::uniform_int_distribution<UINT>(low, high)(generator);
    };
    const ULONGLONG tick = GetTickCount64();
    GraphicsPath lettering(FillModeWinding);
    for (auto* part : { &display.lit, &display.unlit, &display.indicators })
        if (part->GetPointCount()) lettering.AddPath(part, FALSE);
    RectF bounds, numberBounds;
    lettering.GetBounds(&bounds);
    display.lit.GetBounds(&numberBounds);
    if (!nextGlitch) nextGlitch = tick + random(2500, 7000);
    if (digitalGlitchRequested || tick >= nextGlitch) {
        const bool fullDisplay = digitalGlitchRequested;
        digitalGlitchRequested = false;
        glitchStart = tick;
        duration = random(80, 160);
        nextGlitch = tick + duration + random(2500, 7000);
        stripes.clear();
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        for (int attempt = 0; attempt < 64; ++attempt) {
            const PointF point = fullDisplay ? PointF(bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2) :
                PointF(bounds.X + bounds.Width * unit(generator),
                bounds.Y + bounds.Height * unit(generator));
            if (!fullDisplay && !lettering.IsVisible(point)) continue;
            center = point;
            radius = fullDisplay ? std::hypot(bounds.Width, bounds.Height) / 2 + 8 :
                numberBounds.Height * static_cast<float>(random(50, 60)) / 100.0f;
            const float bottom = center.Y + radius;
            for (float top = center.Y - radius; top < bottom;) {
                const float height = std::min(static_cast<float>(random(5, 15)) / 10.0f, bottom - top);
                const float magnitude = static_cast<float>(random(5, 25)) / 10.0f * (fullDisplay ? 3.0f : 1.0f);
                stripes.push_back({ top, height, random(0, 1) ? magnitude : -magnitude });
                top += height;
            }
            break;
        }
    }
    const ULONGLONG elapsed = tick - glitchStart;
    if (stripes.empty() || elapsed >= duration) {
        DrawDigitalFace(graphics, display);
        return;
    }
    DrawDigitalGlitch(graphics, display, bounds, center, radius, stripes);
}

void DrawDigitalTime(Graphics& graphics, const SYSTEMTIME& now) {
    constexpr UINT alertMasks[] = { 0x077, 0x038, 0x079, 0x173, 0x281 }; // A, L, E, R, T
    const int hour = use24Hour ? now.wHour : (now.wHour + 11) % 12 + 1;
    const int digits[] = { hour / 10, hour % 10, 0, now.wMinute / 10, now.wMinute % 10 };
    DigitalDisplayPaths display;
    for (int cell = 0; cell < 5; ++cell) {
        const float left = cell * 24.0f + 3;
        if (!alarmActive && cell == 2) {
            const bool lit = now.wMilliseconds < 500;
            if (lit || showInactiveSegments) {
                auto& path = lit ? display.lit : display.unlit;
                path.AddRectangle(RectF(left + 7, 8, 4, 4));
                path.AddRectangle(RectF(left + 7, 20, 4, 4));
            }
            continue;
        }
        const UINT mask = alarmActive ? alertMasks[cell] : DigitalSegments::NumberMasks[digits[cell]];
        const UINT available = 0x7Fu | (alarmActive ? alertMasks[cell] : 0);
        for (int segment = 0; segment < 10; ++segment) {
            const bool lit = (mask & (1u << segment)) != 0;
            if (!(available & (1u << segment)) || (!lit && !showInactiveSegments)) continue;
            const auto& part = DigitalSegments::Parts[segment];
            PointF polygon[6];
            for (int vertex = 0; vertex < part.count; ++vertex)
                polygon[vertex] = PointF(left + part.points[vertex].X, part.points[vertex].Y);
            (lit ? display.lit : display.unlit).AddPolygon(polygon, part.count);
        }
    }
    constexpr float slant = DigitalSegments::Slant;
    constexpr float sizeScale = 1.125f;
    Matrix placement(sizeScale, 0, -slant * sizeScale, DigitalSegments::HeightScale * sizeScale,
        clockCenter.X + (-60 + 16 * slant) * sizeScale, clockCenter.Y - 84);
    display.lit.Transform(&placement);
    if (display.unlit.GetPointCount()) display.unlit.Transform(&placement);
    if (!use24Hour) {
        for (int period = 0; period < 2; ++period) {
            const bool lit = (now.wHour >= 12) == (period == 1);
            if (!lit && !showInactiveSegments) continue;
            auto& path = lit ? display.indicators : display.unlit;
            const wchar_t* label = period ? L"PM" : L"AM";
            path.AddString(label, 2, FontFamily::GenericSansSerif(), FontStyleBold, 14.0f,
                PointF(clockCenter.X - 28 + period * 36.0f, clockCenter.Y - 100), StringFormat::GenericTypographic());
        }
    }
    DrawDigitalDisplay(graphics, display);
}

bool Load(const std::wstring& directory) {
    const AnimationLayout config{ directory + L"locate data\\mute-layout.ini" };
    return LoadAnimation(directory, config);
}

void Draw(Graphics& graphics, const SYSTEMTIME& now, bool drawBackground) {
    graphics.SetInterpolationMode(InterpolationModeBilinear);
    const GraphicsState clipState = clockDial->BeginScene(graphics, drawBackground);
    DrawCharacter(graphics);
    DrawDigitalTime(graphics, now);
    clockDial->EndScene(graphics, clipState, drawBackground);
    if (drawBackground && hologramEnabled) DrawHologramOverlay(graphics, *clockDial->frameShape);
}
}
