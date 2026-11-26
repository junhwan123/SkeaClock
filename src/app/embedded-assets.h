#pragma once
#include <shlobj.h>
#include <cstring>
#include "assets.h"

// Runs once at startup. The existing artwork/audio loaders keep their file paths.
bool SameAsset(const std::wstring& path, const BYTE* bytes, DWORD size) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER length = {};
    bool same = GetFileSizeEx(file, &length) && length.QuadPart == size;
    BYTE buffer[32768];
    for (DWORD offset = 0; same && offset < size;) {
        const DWORD count = (size - offset < sizeof(buffer)) ? size - offset : sizeof(buffer);
        DWORD read = 0;
        same = ReadFile(file, buffer, count, &read, nullptr) && read == count &&
            std::memcmp(buffer, bytes + offset, count) == 0;
        offset += count;
    }
    CloseHandle(file);
    return same;
}

bool GetAppDataPaths(std::wstring& directory, std::wstring& source, bool lowResolution) {
    PWSTR local = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local);
    if (FAILED(result)) { CoTaskMemFree(local); return false; }
    directory = std::wstring(local) + L"\\SkeaClock";
    CoTaskMemFree(local);
    source = directory + (lowResolution ? L"\\assets-low-resolution\\" : L"\\assets\\");
    return true;
}

bool UnpackEmbeddedAssets(const std::wstring& source) {
    for (const auto& asset : EmbeddedAssets) {
        HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(asset.id), RT_RCDATA);
        if (!resource) return false;
        const DWORD size = SizeofResource(nullptr, resource);
        const auto bytes = static_cast<const BYTE*>(LockResource(LoadResource(nullptr, resource)));
        if (!bytes || !size) return false;
        const std::wstring path = source + asset.path;
        if (SameAsset(path, bytes, size)) continue;
        const std::wstring parent = path.substr(0, path.find_last_of(L'\\'));
        const int created = SHCreateDirectoryExW(nullptr, parent.c_str(), nullptr);
        if (created != ERROR_SUCCESS && created != ERROR_ALREADY_EXISTS) return false;
        const std::wstring temporary = path + L".tmp";
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0;
        bool saved = WriteFile(file, bytes, size, &written, nullptr) && written == size;
        if (!CloseHandle(file)) saved = false;
        if (saved) saved = MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
        if (!saved) { DeleteFileW(temporary.c_str()); return false; }
    }
    return true;
}
