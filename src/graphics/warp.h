#pragma once

#include "../animation/animation.h"
#include "clock-shape.h"

constexpr float PatWarpRadius = DialRadius * .3f * 3;
constexpr float PatFollowFraction = .15f;
constexpr float MaxInwardCheekPull = .3f;
constexpr float CheekSeamOverlap = SourceImageScale;
constexpr UINT TouchBounceFrameCount = 6;

constexpr int CheekRows = 20, CheekColumns = 14;

PointF ClampInwardCheekPull(PointF pull, float radius) {
    const float inwardLimit = radius * MaxInwardCheekPull;
    if (pull.X > inwardLimit) {
        const float scale = inwardLimit / pull.X;
        pull.X *= scale;
        pull.Y *= scale;
    }
    return pull;
}

// Dimensionless geometry is independent of the current frame and mouse position.
struct CheekWarpGrid {
    float vertical[CheekRows + 1], chord[CheekRows + 1];
    float column[CheekColumns + 1], weight[CheekRows + 1][CheekColumns + 1];
    CheekWarpGrid() {
        for (int col = 0; col <= CheekColumns; ++col)
            column[col] = col / static_cast<float>(CheekColumns);
        for (int row = 0; row <= CheekRows; ++row) {
            vertical[row] = -1.0f + 2.0f * row / CheekRows;
            chord[row] = std::sqrt(std::max(0.0f, 1 - vertical[row] * vertical[row]));
            for (int col = 0; col <= CheekColumns; ++col) {
                const float w = 1 - column[col];
                weight[row][col] = w * w * (3 - 2 * w) * chord[row];
            }
        }
    }
};
const CheekWarpGrid cheekWarpGrid;

struct CheekWarpBuffers {
    std::unique_ptr<Bitmap> source, result, composite;
    PointF compositeOffset;
    const AnimationImage* sourceAnimation = nullptr;
    UINT sourceFrame = UINT_MAX;
    PointF sourceOffset;
    Bitmap* warpedFrame = nullptr;
    PointF appliedPull;
    bool compositeReady = false;
    bool geometryReady = false;
    PointF center;
    float radius = 0;
    PointF original[CheekRows + 1][CheekColumns + 1];

    void Release() {
        source.reset();
        result.reset();
        composite.reset();
        sourceAnimation = nullptr;
        sourceFrame = UINT_MAX;
        warpedFrame = nullptr;
        appliedPull = PointF();
        compositeReady = false;
        geometryReady = false;
    }

    bool Prepare(UINT width, UINT height, const AnimationImage& animation, PointF offset) {
        if (!source || source->GetWidth() != width || source->GetHeight() != height) {
            Release();
            source = std::make_unique<Bitmap>(width, height, PixelFormat32bppPARGB);
            result = std::make_unique<Bitmap>(width, height, PixelFormat32bppPARGB);
            if (source->GetLastStatus() != Ok || result->GetLastStatus() != Ok) {
                Release();
                return false;
            }
        }
        if (sourceAnimation != &animation || sourceFrame != animation.frame ||
            sourceOffset.X != offset.X || sourceOffset.Y != offset.Y) {
            sourceAnimation = nullptr;
            warpedFrame = nullptr;
            appliedPull = PointF();
            compositeReady = false;
            auto* head = animation.Frame();
            if (!head) return false;
            Graphics graphics(source.get());
            graphics.Clear(Color(0, 0, 0, 0));
            graphics.SetCompositingMode(CompositingModeSourceCopy);
            if constexpr (SourceImageScale == 1.0f) {
                if (graphics.DrawImage(head, static_cast<INT>(offset.X), static_cast<INT>(offset.Y),
                    head->GetWidth(), head->GetHeight()) != Ok) return false;
            } else {
                graphics.SetInterpolationMode(InterpolationModeBilinear);
                if (graphics.DrawImage(head, RectF(offset.X, offset.Y,
                    static_cast<float>(head->GetWidth()), static_cast<float>(head->GetHeight())),
                    0, 0, static_cast<float>(head->GetWidth()), static_cast<float>(head->GetHeight()), UnitPixel) != Ok) return false;
            }
            sourceAnimation = &animation;
            sourceFrame = animation.frame;
            sourceOffset = offset;
        }
        return true;
    }

    Bitmap* ComposeLayers(AnimationImage& back, Bitmap* head, AnimationImage& front, PointF frontOffset) {
        // The three layers advance together, so a new head frame invalidates this composite.
        if (compositeReady) return composite.get();
        const PointF crop = front.FrameOffset();
        frontOffset = PointF(frontOffset.X + crop.X, frontOffset.Y + crop.Y);
        Bitmap* frontFrame = front.Frame();
        Bitmap* backFrame = back.Frame();
        if (!frontFrame || !backFrame) return nullptr;
        const INT left = static_cast<INT>(std::floor(std::min(0.0f, frontOffset.X)));
        const INT top = static_cast<INT>(std::floor(std::min(0.0f, frontOffset.Y)));
        const UINT width = static_cast<UINT>(std::ceil(std::max(static_cast<float>(back.GetWidth()),
            frontOffset.X + frontFrame->GetWidth())) - left);
        const UINT height = static_cast<UINT>(std::ceil(std::max(static_cast<float>(back.GetHeight()),
            frontOffset.Y + frontFrame->GetHeight())) - top);
        if (!composite || composite->GetWidth() != width || composite->GetHeight() != height) {
            composite = std::make_unique<Bitmap>(width, height, PixelFormat32bppPARGB);
            if (composite->GetLastStatus() != Ok) {
                composite.reset();
                return nullptr;
            }
        }
        compositeOffset = PointF(static_cast<float>(left), static_cast<float>(top));
        Graphics graphics(composite.get());
        graphics.Clear(Color(0, 0, 0, 0));
        graphics.SetInterpolationMode(SourceImageScale == 1.0f ? InterpolationModeNearestNeighbor : InterpolationModeBilinear);
        auto draw = [&](Bitmap* image, float x, float y) {
            if constexpr (SourceImageScale == 1.0f) {
                return graphics.DrawImage(image, static_cast<INT>(x), static_cast<INT>(y),
                    image->GetWidth(), image->GetHeight());
            } else {
                const float w = static_cast<float>(image->GetWidth()), h = static_cast<float>(image->GetHeight());
                return graphics.DrawImage(image, RectF(x, y, w, h), 0, 0, w, h, UnitPixel);
            }
        };
        const PointF backOffset = back.FrameOffset();
        if (draw(backFrame, backOffset.X - left, backOffset.Y - top) != Ok) return nullptr;
        if (head && draw(head, static_cast<float>(-left), static_cast<float>(-top)) != Ok) return nullptr;
        if (draw(frontFrame, frontOffset.X - left, frontOffset.Y - top) != Ok) return nullptr;
        compositeReady = true;
        return composite.get();
    }

    void PrepareGeometry(PointF newCenter, float newRadius) {
        if (geometryReady && center.X == newCenter.X && center.Y == newCenter.Y && radius == newRadius) return;
        center = newCenter;
        radius = newRadius;
        const float x = center.X - .5f, y = center.Y - .5f;
        for (int row = 0; row <= CheekRows; ++row) {
            const float sourceY = y + cheekWarpGrid.vertical[row] * radius;
            const float left = x - radius * cheekWarpGrid.chord[row];
            const float right = x + radius * cheekWarpGrid.chord[row];
            for (int col = 0; col <= CheekColumns; ++col)
                original[row][col] = PointF(left + (right - left) * cheekWarpGrid.column[col], sourceY);
        }
        geometryReady = true;
    }
};
CheekWarpBuffers cheekWarpBuffers;

float TouchReleasePullScale(UINT frame) {
    static constexpr float scales[TouchBounceFrameCount] = {
        1.0f, .55f, .08f, -.14f, .07f, 0.0f
    };
    return frame < TouchBounceFrameCount ? scales[frame] : 0.0f;
}

Bitmap* WarpedHead(UINT width, UINT height, const AnimationImage& animation, PointF headOffset,
    PointF warpCenter, PointF effectivePull, float warpRadius, bool radial) {
    if (!cheekWarpBuffers.Prepare(width, height, animation, headOffset)) return nullptr;
    if (cheekWarpBuffers.warpedFrame) return cheekWarpBuffers.warpedFrame;
    Bitmap* source = cheekWarpBuffers.source.get();
    Bitmap* result = cheekWarpBuffers.result.get();
    cheekWarpBuffers.warpedFrame = source;
    float pullX = effectivePull.X, pullY = effectivePull.Y;
    if (std::hypot(pullX, pullY) < .01f) return source;
    const float warpCenterX = warpCenter.X - .5f, warpCenterY = warpCenter.Y - .5f;
    {
        Graphics graphics(result);
        graphics.Clear(Color(0, 0, 0, 0));
        graphics.SetCompositingMode(CompositingModeSourceCopy);
        graphics.DrawImage(source, 0, 0, width, height);
    }
    Rect bounds(0, 0, width, height);
    BitmapData sourceData = {}, resultData = {};
    if (source->LockBits(&bounds, ImageLockModeRead, PixelFormat32bppPARGB, &sourceData) != Ok ||
        result->LockBits(&bounds, ImageLockModeWrite, PixelFormat32bppPARGB, &resultData) != Ok) {
        if (sourceData.Scan0) source->UnlockBits(&sourceData);
        return source;
    }
    auto pixel = [&](int x, int y, int channel) -> float {
        if (x < 0 || y < 0 || x >= static_cast<int>(width) || y >= static_cast<int>(height)) return 0;
        const BYTE* row = static_cast<const BYTE*>(sourceData.Scan0) + y * sourceData.Stride;
        return row[x * 4 + channel];
    };
    if (!radial && (pullX <= 0 || std::abs(pullY) > .01f)) {
        for (int y = std::max(0, static_cast<int>(std::floor(warpCenterY - warpRadius)));
            y <= std::min(static_cast<int>(height) - 1, static_cast<int>(std::ceil(warpCenterY + warpRadius))); ++y) {
            const float vertical = (y - warpCenterY) / warpRadius;
            if (std::abs(vertical) >= 1) continue;
            const float chord = std::sqrt(1 - vertical * vertical);
            const int first = std::max(0, static_cast<int>(std::floor(warpCenterX - warpRadius * chord)));
            const int last = std::min(static_cast<int>(width) - 1,
                static_cast<int>(std::floor(warpCenterX + warpRadius * chord - CheekSeamOverlap)));
            BYTE* output = static_cast<BYTE*>(resultData.Scan0) + y * resultData.Stride;
            for (int x = first; x <= last; ++x)
                output[x * 4] = output[x * 4 + 1] = output[x * 4 + 2] = output[x * 4 + 3] = 0;
        }
    }
    struct WarpVertex { float x, y, sourceX, sourceY; };
    constexpr int Rows = CheekRows, Columns = CheekColumns;
    WarpVertex mesh[Rows + 1][Columns + 1] = {};
    cheekWarpBuffers.PrepareGeometry(warpCenter, warpRadius);
    for (int row = 0; row <= Rows; ++row) {
        for (int column = 0; column <= Columns; ++column) {
            const PointF point = cheekWarpBuffers.original[row][column];
            float weight = cheekWarpGrid.weight[row][column];
            if (radial) {
                const float dx = (point.X - warpCenterX) / warpRadius;
                const float dy = (point.Y - warpCenterY) / warpRadius;
                const float falloff = std::max(0.0f, 1 - dx * dx - dy * dy);
                weight = falloff * falloff;
            }
            mesh[row][column] = { point.X + pullX * weight, point.Y + pullY * weight, point.X, point.Y };
        }
    }
    auto edge = [](const WarpVertex& a, const WarpVertex& b, float x, float y) {
        return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
    };
    auto drawTriangle = [&](const WarpVertex& a, const WarpVertex& b, const WarpVertex& c) {
        const float area = edge(a, b, c.x, c.y);
        if (std::abs(area) < .001f) return;
        const int firstX = std::max(0, static_cast<int>(std::floor(std::min({ a.x, b.x, c.x }))));
        const int firstY = std::max(0, static_cast<int>(std::floor(std::min({ a.y, b.y, c.y }))));
        const int lastX = std::min(static_cast<int>(width) - 1,
            static_cast<int>(std::ceil(std::max({ a.x, b.x, c.x }))));
        const int lastY = std::min(static_cast<int>(height) - 1,
            static_cast<int>(std::ceil(std::max({ a.y, b.y, c.y }))));
        for (int y = firstY; y <= lastY; ++y) {
            BYTE* output = static_cast<BYTE*>(resultData.Scan0) + y * resultData.Stride;
            for (int x = firstX; x <= lastX; ++x) {
                const float px = static_cast<float>(x), py = static_cast<float>(y);
                const float wa = edge(b, c, px, py) / area;
                const float wb = edge(c, a, px, py) / area;
                const float wc = 1 - wa - wb;
                if (wa < -.001f || wb < -.001f || wc < -.001f) continue;
                const float sourceX = a.sourceX * wa + b.sourceX * wb + c.sourceX * wc;
                const float sourceY = a.sourceY * wa + b.sourceY * wb + c.sourceY * wc;
                const int left = static_cast<int>(std::floor(sourceX));
                const int top = static_cast<int>(std::floor(sourceY));
                const float fx = sourceX - left, fy = sourceY - top;
                float sample[4];
                for (int channel = 0; channel < 4; ++channel) {
                    const float upper = pixel(left, top, channel) * (1 - fx) + pixel(left + 1, top, channel) * fx;
                    const float lower = pixel(left, top + 1, channel) * (1 - fx) + pixel(left + 1, top + 1, channel) * fx;
                    sample[channel] = std::clamp(upper * (1 - fy) + lower * fy, 0.0f, 255.0f);
                }
                const float remaining = radial ? 0.0f : 1 - sample[3] / 255.0f;
                for (int channel = 0; channel < 4; ++channel)
                    output[x * 4 + channel] = static_cast<BYTE>(std::clamp(
                        sample[channel] + output[x * 4 + channel] * remaining, 0.0f, 255.0f));
            }
        }
    };
    for (int row = 0; row < Rows; ++row) for (int column = 0; column < Columns; ++column) {
        drawTriangle(mesh[row][column], mesh[row + 1][column], mesh[row + 1][column + 1]);
        drawTriangle(mesh[row][column], mesh[row + 1][column + 1], mesh[row][column + 1]);
    }
    source->UnlockBits(&sourceData);
    result->UnlockBits(&resultData);
    cheekWarpBuffers.appliedPull = effectivePull;
    cheekWarpBuffers.warpedFrame = result;
    return result;
}

void DrawWarpedLayers(Graphics& graphics, AnimationImage& back, AnimationImage& head,
    AnimationImage& front, PointF position, float scale, PointF center, PointF pull, float radius) {
    Bitmap* warped = head.Frame() ? WarpedHead(head.GetWidth(), head.GetHeight(), head,
        head.FrameOffset(), center, pull, radius, false) : nullptr;
    Bitmap* composite = warped ? cheekWarpBuffers.ComposeLayers(back, warped, front, PointF()) : nullptr;
    if (composite) {
        DrawAnimationFrame(graphics, composite, position, cheekWarpBuffers.compositeOffset, scale);
        return;
    }
    DrawAnimationFrame(graphics, back.Frame(), position, back.FrameOffset(), scale);
    DrawAnimationFrame(graphics, warped ? warped : head.Frame(), position,
        warped ? PointF() : head.FrameOffset(), scale);
    DrawAnimationFrame(graphics, front.Frame(), position, front.FrameOffset(), scale);
}
