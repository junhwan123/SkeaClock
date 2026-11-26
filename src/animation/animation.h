#pragma once

#include <algorithm>
#include <cmath>
#include "animation-sheet.h"

constexpr float SourceImageScale = SKEA_IMAGE_SCALE;

struct AnimationFrameRegion {
    AnimationSheet* sheet = nullptr;
    size_t view = 0;
    PointF offset;
};

struct AnimationImage {
    std::vector<AnimationFrameRegion> regions;
    std::vector<std::vector<AnimationFrameRegion>> continuations;
    std::vector<UINT> frameEnds;
    UINT width = 0, height = 0, firstFrame = 0, storedFrames = 0, frame = 0;

    UINT GetWidth() const { return width; }
    UINT GetHeight() const { return height; }
    UINT GetFrameCount() const { return static_cast<UINT>(frameEnds.size()); }

    const AnimationFrameRegion& Source(UINT index) const {
        for (const auto& region : continuations[index])
            if (region.sheet->decoded) return region;
        return regions[index];
    }

    PointF FrameOffset() const { return Source(frame).offset; }
    Bitmap* Frame() const {
        const auto& region = Source(frame);
        return region.sheet && region.sheet->decoded ? region.sheet->decoded->views[region.view].get() : nullptr;
    }

    bool ReadRegion(const wchar_t* text, AnimationSheet* sheet, AnimationFrameRegion& region) const {
        if (!sheet) return false;
        Rect bounds;
        int x = -1, y = -1;
        if (swscanf_s(text, L"%d,%d,%d,%d,%d,%d", &bounds.X, &bounds.Y,
            &bounds.Width, &bounds.Height, &x, &y) != 6 || bounds.X < 0 || bounds.Y < 0 ||
            bounds.Width <= 0 || bounds.Height <= 0 || bounds.Width > static_cast<int>(width) ||
            bounds.Height > static_cast<int>(height) || x < 0 || y < 0 ||
            x > static_cast<int>(width) - bounds.Width || y > static_cast<int>(height) - bounds.Height) return false;
        region = { sheet, sheet->regions.size(), PointF(static_cast<float>(x), static_cast<float>(y)) };
        sheet->regions.push_back(bounds);
        return true;
    }

    bool Load(const std::wstring& directory, const std::wstring& config, const wchar_t* section) {
        auto read = [&](const wchar_t* key) {
            return static_cast<int>(GetPrivateProfileIntW(section, key, -1, config.c_str()));
        };
        const int w = read(L"width"), h = read(L"height"), count = read(L"frameCount");
        const int first = read(L"firstFrame"), stored = read(L"sheetFrames"), size = read(L"chunkSize");
        const int chunkCount = read(L"chunks");
        if (w <= 0 || h <= 0 || count <= 0 || first < 0 || first >= count || size <= 0 ||
            stored <= 0 || stored > count - first || chunkCount != (stored - 1) / size + 1) return false;
        width = static_cast<UINT>(w);
        height = static_cast<UINT>(h);
        firstFrame = frame = static_cast<UINT>(first);
        storedFrames = static_cast<UINT>(stored);
        regions.resize(count);
        continuations.resize(count);
        std::vector<AnimationSheet*> sheets;
        for (int i = 0; i < chunkCount; ++i) {
            wchar_t file[MAX_PATH] = {};
            const std::wstring key = i ? L"file" + std::to_wstring(i) : L"file";
            GetPrivateProfileStringW(section, key.c_str(), L"", file, MAX_PATH, config.c_str());
            auto* sheet = FindAnimationSheet(directory + file);
            if (!sheet) return false;
            sheets.push_back(sheet);
        }
        UINT duration = 0;
        for (int i = 0; i < count; ++i) {
            const int delay = read((L"delay" + std::to_wstring(i)).c_str());
            if (delay < 10 || delay > 600000 || duration > MAXUINT - static_cast<UINT>(delay)) return false;
            duration += static_cast<UINT>(delay);
            frameEnds.push_back(duration);
        }
        for (int i = first; i < first + stored; ++i) {
            wchar_t value[128] = {};
            const std::wstring key = L"rect" + std::to_wstring(i);
            GetPrivateProfileStringW(section, key.c_str(), L"", value, 128, config.c_str());
            if (!ReadRegion(value, sheets[(i - first) / size], regions[i])) return false;
        }
        // Older resizing tools preserve extra keys while rebuilding only the primary cells.
        const UINT aliasWidth = GetPrivateProfileIntW(section, L"aliasWidth", width, config.c_str());
        const UINT aliases = aliasWidth == width ? GetPrivateProfileIntW(section, L"aliases", 0, config.c_str()) : 0;
        for (UINT i = 0; i < aliases; ++i) {
            wchar_t value[512] = {};
            const std::wstring key = L"alias" + std::to_wstring(i);
            GetPrivateProfileStringW(section, key.c_str(), L"", value, 512, config.c_str());
            const std::wstring text(value);
            const size_t pathStart = text.find(L'|');
            const size_t pathEnd = pathStart == std::wstring::npos ? pathStart : text.find(L'|', pathStart + 1);
            UINT index = UINT_MAX;
            if (pathEnd == std::wstring::npos ||
                swscanf_s(value, L"%u", &index) != 1 || index < firstFrame || index - firstFrame >= storedFrames)
                return false;
            auto* sheet = FindAnimationSheet(directory + text.substr(pathStart + 1, pathEnd - pathStart - 1));
            AnimationFrameRegion region;
            if (!ReadRegion(text.c_str() + pathEnd + 1, sheet, region)) return false;
            continuations[index].push_back(region);
        }
        return true;
    }

    bool Prepare(UINT index, bool immediate = false) const {
        if (index < firstFrame || index - firstFrame >= storedFrames) return false;
        return Source(index).sheet->Prepare(immediate);
    }

    Status SelectFrame(UINT index, bool immediate = false) {
        if (!Prepare(index, immediate)) {
            Prepare(frame);
            return GenericError;
        }
        frame = index;
        return Ok;
    }

    void PrepareNext(const AnimationImage* next, UINT entry) const {
        const auto* current = Source(frame).sheet;
        for (UINT i = frame + 1; i < firstFrame + storedFrames; ++i) {
            auto* sheet = Source(i).sheet;
            if (sheet == current) continue;
            sheet->Prepare();
            return;
        }
        if (!next) return;
        for (UINT i = entry; i < next->firstFrame + next->storedFrames; ++i) {
            auto* sheet = next->Source(i).sheet;
            if (sheet == current) continue;
            sheet->Prepare();
            return;
        }
    }
};

constexpr ULONGLONG AnimationFps = 30;
constexpr ULONGLONG FrameUnits = 1000;
bool prepareAlarmAnimation = false;

struct AnimationTiming {
    UINT first = 0;
    UINT frame = 0;
    ULONGLONG start = 0;

    void Begin(UINT firstFrame, ULONGLONG tick) {
        first = firstFrame;
        start = tick;
    }

    ULONGLONG Elapsed(ULONGLONG now) const { return now * AnimationFps - start; }
    ULONGLONG Position(ULONGLONG now) const { return first + Elapsed(now) / FrameUnits; }
    ULONGLONG End(UINT count) const { return start + (count - first) * FrameUnits; }
    UINT Delay(ULONGLONG now) const {
        return static_cast<UINT>((FrameUnits - Elapsed(now) % FrameUnits + AnimationFps - 1) / AnimationFps);
    }
};

template<class Clip>
struct AnimationTransition {
    Clip clip;
    UINT frame = 0;
};

bool PrepareLayerFrames(const std::unique_ptr<AnimationImage> (&layers)[3], UINT frame, bool immediate = false) {
    bool ready = true;
    for (const auto& image : layers) if (image && !image->Prepare(frame, immediate)) ready = false;
    return ready;
}

bool SelectLayerFrames(const std::unique_ptr<AnimationImage> (&layers)[3], UINT frame, bool immediate = false) {
    const bool ready = PrepareLayerFrames(layers, frame, immediate);
    for (const auto& image : layers) {
        if (!image) continue;
        if (ready) image->frame = frame;
        else image->Prepare(image->frame);
    }
    return ready;
}

void PrepareFollowingLayers(const std::unique_ptr<AnimationImage> (&layers)[3],
    const std::unique_ptr<AnimationImage> (&next)[3], UINT entry) {
    for (int layer = 0; layer < 3; ++layer)
        if (layers[layer]) layers[layer]->PrepareNext(next[layer].get(), entry);
}

bool PointInImage(PointF point, UINT width, UINT height) {
    return point.X >= 0 && point.Y >= 0 && point.X < width && point.Y < height;
}

struct AnimationLayout {
    std::wstring path;

    std::wstring Read(const wchar_t* section, const wchar_t* key) const {
        wchar_t value[MAX_PATH] = {};
        GetPrivateProfileStringW(section, key, L"", value, MAX_PATH, path.c_str());
        return value;
    }

    float Number(const wchar_t* section, const wchar_t* key, float fallback = std::nanf("")) const {
        const auto text = Read(section, key);
        if (text.empty()) return fallback;
        float value = std::nanf("");
        swscanf_s(text.c_str(), L"%f", &value);
        return value;
    }

    bool FrameCenters(const wchar_t* section, const AnimationImage& image, std::vector<PointF>& centers) const {
        centers.clear();
        const UINT count = image.GetFrameCount();
        centers.reserve(count);
        for (UINT frame = 0; frame < count; ++frame) {
            const auto text = Read(section, (L"frame" + std::to_wstring(frame)).c_str());
            float x = -1, y = -1;
            if (swscanf_s(text.c_str(), L"%f,%f", &x, &y) != 2) return false;
            const PointF center(x + .5f, y + .5f);
            if (!PointInImage(center, image.GetWidth(), image.GetHeight())) return false;
            centers.push_back(center);
        }
        return true;
    }
};

void DrawAnimationFrame(Graphics& graphics, Bitmap* image, PointF origin, PointF offset, float scale) {
    if (image) graphics.DrawImage(image,
        RectF(origin.X + offset.X * scale, origin.Y + offset.Y * scale,
            image->GetWidth() * scale, image->GetHeight() * scale),
        0, 0, static_cast<float>(image->GetWidth()), static_cast<float>(image->GetHeight()), UnitPixel);
}
