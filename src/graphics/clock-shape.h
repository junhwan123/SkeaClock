#pragma once

#include <memory>
#include "../clock/character.h"

constexpr float Pi = 3.14159265358979323846f;
constexpr float DialRadius = 108.0f;
constexpr float DialFrameWidth = 6.0f;
constexpr int artworkWidth = static_cast<int>(DialRadius * 2 + 24);
constexpr int artworkHeight = artworkWidth;
const PointF clockCenter(DialRadius + 12, DialRadius + 12);

struct ClockHand {
    std::unique_ptr<Bitmap> image;
    PointF center, tip;
    float nativeAngle = 0;
};

struct DialPath { const PointF* points; const BYTE* types; INT count; };
struct DialTick { PointF start, end; };
const Gdiplus::PointF ClipPoints[] = {
    { 108.0f, 0.0f },
    { 108.0f, 59.6467476f },
    { 59.6467476f, 108.0f },
    { 0.0f, 108.0f },
    { -59.64675f, 108.0f },
    { -108.0f, 59.6467476f },
    { -108.0f, 0.0f },
    { -108.0f, -59.64675f },
    { -59.64675f, -108.0f },
    { 0.0f, -108.0f },
    { 59.6467476f, -108.0f },
    { 108.0f, -59.64675f },
    { 108.0f, 0.0f },
};
const BYTE ClipTypes[] = { 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 131 };
const DialPath DialClip = { ClipPoints, ClipTypes, 13 };

void AddDialWithRoundedCorner(GraphicsPath& shape, float radius) {
    constexpr float cornerRadius = 20.0f;
    constexpr float cornerDiameter = cornerRadius * 2;
    const float right = clockCenter.X + radius;
    const float bottom = clockCenter.Y + radius;
    shape.AddLine(right, clockCenter.Y, right, bottom - cornerRadius);
    shape.AddArc(right - cornerDiameter, bottom - cornerDiameter, cornerDiameter, cornerDiameter, 0.0f, 90.0f);
    shape.AddLine(right - cornerRadius, bottom, clockCenter.X, bottom);
    shape.AddArc(clockCenter.X - radius, clockCenter.Y - radius, radius * 2, radius * 2, 90.0f, 270.0f);
    shape.CloseFigure();
}

RectF DialCornerCircleBounds(float outerRadius, float inset) {
    constexpr float diameter = 36.0f;
    const float offset = outerRadius - inset - diameter;
    return RectF(clockCenter.X + offset, clockCenter.Y + offset, diameter, diameter);
}

// Geometry stays in artwork coordinates; the renderer scales it for each window size.
struct ClockDial {
    std::unique_ptr<GraphicsPath> clip, sceneClip, frameShape, playerFrameShape;
    std::unique_ptr<GraphicsPath> numbers[12];
    PointF tickStart[60], tickEnd[60];
    SolidBrush background{ ClockCharacters[selectedCharacter].background };
    SolidBrush border{ ClockCharacters[selectedCharacter].border };
    SolidBrush ink{ Color(255, 50, 63, 72) };
    Pen majorOutline{ Color(255, 255, 255, 255), 5.0f };
    Pen minorOutline{ Color(255, 255, 255, 255), 3.5f };
    Pen majorTick{ Color(255, 67, 79, 88), 3.0f };
    Pen minorTick{ Color(220, 84, 99, 109), 1.5f };
    Pen numberOutline{ Color(255, 255, 255, 255), 2.5f };

    static std::unique_ptr<GraphicsPath> LoadPath(const DialPath& data, PointF center) {
        auto path = std::make_unique<GraphicsPath>(data.points, data.types, data.count);
        Matrix placement(1, 0, 0, 1, center.X, center.Y);
        if (path->GetLastStatus() != Ok || path->Transform(&placement) != Ok) return nullptr;
        return path;
    }

    bool InitializeFrame(PointF center) {
        constexpr float outerRadius = DialRadius + DialFrameWidth;
        frameShape = std::make_unique<GraphicsPath>(FillModeAlternate);
        frameShape->AddEllipse(center.X - outerRadius, center.Y - outerRadius, outerRadius * 2, outerRadius * 2);
        frameShape->AddPath(clip.get(), FALSE);
        if (IsCoffeCharacter()) {
            playerFrameShape = std::make_unique<GraphicsPath>(FillModeAlternate);
            AddDialWithRoundedCorner(*playerFrameShape, outerRadius);
            playerFrameShape->AddPath(clip.get(), FALSE);
            if (playerFrameShape->GetLastStatus() != Ok) return false;
        }

        // Draw underneath the frame so its antialiased inner edge covers the scene clip.
        constexpr float sceneRadius = DialRadius + DialFrameWidth / 2;
        sceneClip = std::make_unique<GraphicsPath>();
        sceneClip->AddEllipse(center.X - sceneRadius, center.Y - sceneRadius, sceneRadius * 2, sceneRadius * 2);
        return frameShape->GetLastStatus() == Ok && sceneClip->GetLastStatus() == Ok;
    }

    bool Initialize(PointF center) {
        clip = LoadPath(DialClip, center);
        return clip && (IsKommyCharacter() || InitializeFrame(center));
    }

    bool InitializeMarkings(const DialPath (&numberData)[12], const DialTick (&tickData)[60]) {
        const PointF center = clockCenter;
        for (int i = 0; i < 60; ++i) {
            const auto& tick = tickData[i];
            tickStart[i] = PointF(center.X + tick.start.X, center.Y + tick.start.Y);
            tickEnd[i] = PointF(center.X + tick.end.X, center.Y + tick.end.Y);
        }
        numberOutline.SetLineJoin(LineJoinRound);
        for (int i = 0; i < 12; ++i) {
            numbers[i] = LoadPath(numberData[i], center);
            if (!numbers[i]) return false;
        }
        return true;
    }

    GraphicsState BeginScene(Graphics& graphics, bool drawBackground) const {
        const GraphicsState state = graphics.Save();
        graphics.SetClip(drawBackground ? sceneClip.get() : clip.get(), CombineModeReplace);
        if (drawBackground) graphics.FillRectangle(&background, 0, 0, artworkWidth, artworkHeight);
        return state;
    }

    void EndScene(Graphics& graphics, GraphicsState state, bool drawBackground, bool withPlayerCorner = false) const {
        graphics.Restore(state);
        if (drawBackground) {
            const auto* shape = withPlayerCorner && playerFrameShape ? playerFrameShape.get() : frameShape.get();
            graphics.FillPath(&border, shape);
        }
    }
};

std::unique_ptr<ClockDial> clockDial;

GraphicsState ClipToDial(Graphics& graphics) {
    return clockDial->BeginScene(graphics, false);
}
