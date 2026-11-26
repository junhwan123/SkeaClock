#pragma once

#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct AnimationChunk {
    std::vector<BYTE> pixels;
    std::vector<std::unique_ptr<Bitmap>> views;

    bool Load(const std::wstring& path, const std::vector<Rect>& regions) {
        Bitmap encoded(path.c_str());
        const UINT width = encoded.GetWidth(), height = encoded.GetHeight();
        if (encoded.GetLastStatus() != Ok || !width || !height || width > INT_MAX / 4 ||
            height > INT_MAX / (width * 4)) return false;
        for (const Rect& bounds : regions) {
            if (bounds.X > static_cast<INT>(width) - bounds.Width ||
                bounds.Y > static_cast<INT>(height) - bounds.Height) return false;
        }
        const UINT stride = width * 4;
        // The last view may use a stride that extends past the visible pixels.
        pixels.resize(static_cast<size_t>(stride) * (height + 1));
        Bitmap sheet(width, height, stride, PixelFormat32bppPARGB, pixels.data());
        if (sheet.GetLastStatus() != Ok) return false;
        {
            Graphics graphics(&sheet);
            graphics.SetCompositingMode(CompositingModeSourceCopy);
            if (graphics.DrawImage(&encoded, Rect(0, 0, width, height), 0, 0, width, height, UnitPixel) != Ok)
                return false;
        }
        for (const Rect& bounds : regions) {
            const size_t offset = static_cast<size_t>(bounds.Y) * stride + bounds.X * 4;
            auto view = std::make_unique<Bitmap>(bounds.Width, bounds.Height, stride,
                PixelFormat32bppPARGB, pixels.data() + offset);
            if (view->GetLastStatus() != Ok) return false;
            views.push_back(std::move(view));
        }
        return true;
    }
};

struct AnimationSheet {
    explicit AnimationSheet(const std::wstring& filename) : path(filename) {}

    std::wstring path;
    std::vector<Rect> regions;
    std::unique_ptr<AnimationChunk> decoded;
    bool needed = false;
    // Wait for the loader before destroying the path or regions it reads.
    std::future<std::unique_ptr<AnimationChunk>> loading;

    std::unique_ptr<AnimationChunk> Decode() const {
        auto chunk = std::make_unique<AnimationChunk>();
        if (!chunk->Load(path, regions)) return nullptr;
        return chunk;
    }

    void Collect() {
        if (loading.valid() && loading.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            decoded = loading.get();
    }

    bool Prepare(bool immediate = false) {
        needed = true;
        Collect();
        if (!decoded) {
            if (immediate) decoded = loading.valid() ? loading.get() : Decode();
            else if (!loading.valid()) loading = std::async(std::launch::async, [this] { return Decode(); });
        }
        return decoded != nullptr;
    }
};

std::map<std::wstring, AnimationSheet> animationSheets;

AnimationSheet* FindAnimationSheet(const std::wstring& path) {
    const auto existing = animationSheets.find(path);
    if (existing != animationSheets.end()) return &existing->second;
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) return nullptr;
    return &animationSheets.try_emplace(path, path).first->second;
}

struct AnimationPrefetchPass {
    AnimationPrefetchPass() {
        for (auto& item : animationSheets) {
            item.second.Collect();
            item.second.needed = false;
        }
    }

    ~AnimationPrefetchPass() {
        for (auto& item : animationSheets)
            if (!item.second.needed) item.second.decoded.reset();
    }
};
