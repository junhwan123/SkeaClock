#pragma once

namespace DigitalSegments {
constexpr UINT NumberMasks[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
constexpr float Slant = .16f, HeightScale = 1.125f;
const struct { int count; PointF points[6]; } Parts[] = {
    { 6, { PointF(1.5f, .8f), PointF(2.3f, 0), PointF(15.7f, 0), PointF(16.5f, .8f),
        PointF(13.3f, 4), PointF(4.7f, 4) } },
    { 6, { PointF(17.2f, 1.5f), PointF(18, 2.3f), PointF(18, 14.1f), PointF(17.2f, 14.9f),
        PointF(14, 13.3f), PointF(14, 4.7f) } },
    { 6, { PointF(17.2f, 17.1f), PointF(18, 17.9f), PointF(18, 29.7f), PointF(17.2f, 30.5f),
        PointF(14, 27.3f), PointF(14, 18.7f) } },
    { 6, { PointF(4.7f, 28), PointF(13.3f, 28), PointF(16.5f, 31.2f), PointF(15.7f, 32),
        PointF(2.3f, 32), PointF(1.5f, 31.2f) } },
    { 6, { PointF(.8f, 17.1f), PointF(4, 18.7f), PointF(4, 27.3f), PointF(.8f, 30.5f),
        PointF(0, 29.7f), PointF(0, 17.9f) } },
    { 6, { PointF(.8f, 1.5f), PointF(4, 4.7f), PointF(4, 13.3f), PointF(.8f, 14.9f),
        PointF(0, 14.1f), PointF(0, 2.3f) } },
    { 6, { PointF(.7f, 16), PointF(4.7f, 14), PointF(13.3f, 14), PointF(17.3f, 16),
        PointF(13.3f, 18), PointF(4.7f, 18) } },
    { 6, { PointF(9, 5), PointF(11, 7), PointF(11, 15), PointF(9, 17),
        PointF(7, 15), PointF(7, 7) } },
    { 6, { PointF(10, 19), PointF(12.8f, 20.6f), PointF(17.2f, 29.7f), PointF(16, 31.3f),
        PointF(13.2f, 29.7f), PointF(8.8f, 20.6f) } },
    { 6, { PointF(9, 19), PointF(11, 21), PointF(11, 29), PointF(9, 31),
        PointF(7, 29), PointF(7, 21) } }
};
}
