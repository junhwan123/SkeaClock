#pragma once
#include "sources/clock-dial-data.h"

static_assert(ClockDialData::Radius == DialRadius, "Regenerate clock-dial-data.h after changing DialRadius.");

// Geometry stays in artwork coordinates; the renderer scales it for each window size.
struct ClockDial {
    std::unique_ptr<GraphicsPath> clip;
    std::unique_ptr<GraphicsPath> numbers[12];
    PointF tickStart[60], tickEnd[60];
    SolidBrush background{ Color(255, 0xDF, 0xF7, 0xF5) };
    SolidBrush ink{ Color(255, 50, 63, 72) };
    Pen majorOutline{ Color(255, 255, 255, 255), 5.0f };
    Pen minorOutline{ Color(255, 255, 255, 255), 3.5f };
    Pen majorTick{ Color(255, 67, 79, 88), 3.0f };
    Pen minorTick{ Color(220, 84, 99, 109), 1.5f };
    Pen numberOutline{ Color(255, 255, 255, 255), 2.5f };

    static std::unique_ptr<GraphicsPath> LoadPath(const ClockDialData::Path& data, PointF center) {
        auto path = std::make_unique<GraphicsPath>(data.points, data.types, data.count);
        Matrix placement(1, 0, 0, 1, center.X, center.Y);
        if (path->GetLastStatus() != Ok || path->Transform(&placement) != Ok) return nullptr;
        return path;
    }

    bool Initialize(PointF center) {
        clip = LoadPath(ClockDialData::Clip, center);
        if (!clip) return false;
        for (int i = 0; i < 60; ++i) {
            const auto& tick = ClockDialData::Ticks[i];
            tickStart[i] = PointF(center.X + tick.start.X, center.Y + tick.start.Y);
            tickEnd[i] = PointF(center.X + tick.end.X, center.Y + tick.end.Y);
        }
        numberOutline.SetLineJoin(LineJoinRound);
        for (int i = 0; i < 12; ++i) {
            numbers[i] = LoadPath(ClockDialData::Numbers[i], center);
            if (!numbers[i]) return false;
        }
        return true;
    }
};

std::unique_ptr<ClockDial> clockDial;
