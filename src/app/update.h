#pragma once

#include <atomic>
#include <cstdlib>
#include "error.h"

#include <winhttp.h>
#include <array>
#include <chrono>
#include <memory>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "version.lib")

constexpr wchar_t GithubProjectUrl[] = L"https://github.com/junhwan123/SkeaClock";
constexpr wchar_t GithubLatestReleaseUrl[] = L"https://github.com/junhwan123/SkeaClock/releases/latest";
constexpr long long UpdateCheckInterval = 7LL * 24 * 60 * 60;

long long UpdateCheckTime() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

using ExecutableVersion = std::array<unsigned, 3>;

bool ParseVersion(std::wstring_view tag, ExecutableVersion& version) {
    size_t position = !tag.empty() && (tag[0] == L'v' || tag[0] == L'V') ? 1 : 0;
    version = {};
    for (size_t part = 0; part < version.size(); ++part) {
        const size_t start = position;
        unsigned value = 0;
        while (position < tag.size() && tag[position] >= L'0' && tag[position] <= L'9') {
            value = value * 10 + (tag[position++] - L'0');
            if (value > 65535) return false;
        }
        if (position == start) return false;
        version[part] = value;
        if (part + 1 < version.size() && (position == tag.size() || tag[position++] != L'.')) return false;
    }
    if (position == tag.size()) return true;
    // Older settings may include a numeric build revision.
    return tag[position] == L'.' && position + 1 < tag.size() &&
        tag.find_first_not_of(L"0123456789", position + 1) == std::wstring_view::npos;
}

bool ReadExecutableVersion(ExecutableVersion& version) {
    wchar_t executable[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    const DWORD size = GetFileVersionInfoSizeW(executable, nullptr);
    if (!size) return false;
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(executable, 0, size, data.data())) return false;
    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &infoSize) ||
        infoSize < sizeof(*info) || info->dwSignature != 0xFEEF04BD) return false;
    version = { HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
        HIWORD(info->dwFileVersionLS) };
    return true;
}

std::wstring RequestLatestUpdateTag(const std::wstring& path) {
    using HttpHandle = std::unique_ptr<void, decltype(&WinHttpCloseHandle)>;
    HttpHandle session(WinHttpOpen(L"SkeaClock", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0), WinHttpCloseHandle);
    if (!session || !WinHttpSetTimeouts(session.get(), 5000, 5000, 5000, 5000)) return {};
    HttpHandle connection(WinHttpConnect(session.get(), L"github.com", INTERNET_DEFAULT_HTTPS_PORT, 0),
        WinHttpCloseHandle);
    if (!connection) return {};
    HttpHandle request(WinHttpOpenRequest(connection.get(), L"HEAD", L"/junhwan123/SkeaClock/releases/latest",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE), WinHttpCloseHandle);
    if (!request) return {};
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS;
    if (!WinHttpSetOption(request.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled))) return {};

    // Persist the attempt before waiting for a response, including failed requests.
    const std::wstring checkedAt = std::to_wstring(UpdateCheckTime());
    if (!WritePrivateProfileStringW(L"updates", L"lastCheck", checkedAt.c_str(), path.c_str())) return {};
    if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request.get(), nullptr)) return {};
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) || status < 300 || status >= 400)
        return {};
    wchar_t location[2048] = {};
    size = sizeof(location);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
        location, &size, WINHTTP_NO_HEADER_INDEX)) return {};
    const std::wstring url = location;
    const std::wstring prefix = std::wstring(GithubProjectUrl) + L"/releases/tag/";
    if (url.compare(0, prefix.size(), prefix) != 0) return {};
    return url.substr(prefix.size());
}

constexpr int AutoUpdateCheck = 112, GithubVisit = 113;
bool autoCheckUpdates = true;

struct UpdateCheck {
    std::wstring path, tag;
    std::atomic<bool> complete{ false };
};
std::shared_ptr<UpdateCheck> updateCheck;

void CALLBACK LoadLatestUpdate(PTP_CALLBACK_INSTANCE, void* context) {
    std::unique_ptr<std::shared_ptr<UpdateCheck>> holder(static_cast<std::shared_ptr<UpdateCheck>*>(context));
    const auto job = *holder;
    try {
        job->tag = RequestLatestUpdateTag(job->path);
    } catch (...) {
        job->tag.clear();
    }
    job->complete.store(true, std::memory_order_release);
}

void BeginUpdateCheck() {
    if (!autoCheckUpdates || updateCheck) return;
    wchar_t previous[32] = {};
    GetPrivateProfileStringW(L"updates", L"lastCheck", L"0", previous, 32, settingsPath.c_str());
    const long long lastCheck = _wcstoi64(previous, nullptr, 10);
    if (lastCheck > 0 && UpdateCheckTime() - lastCheck < UpdateCheckInterval) return;
    auto job = std::make_shared<UpdateCheck>();
    job->path = settingsPath;
    auto holder = std::make_unique<std::shared_ptr<UpdateCheck>>(job);
    updateCheck = job;
    if (TrySubmitThreadpoolCallback(LoadLatestUpdate, holder.get(), nullptr)) holder.release();
    else updateCheck.reset();
}

void CheckUpdateResult(HWND window) {
    if (!updateCheck || !updateCheck->complete.load(std::memory_order_acquire)) return;
    const auto job = std::move(updateCheck);
    if (!autoCheckUpdates || job->tag.empty()) return;
    ExecutableVersion latest = {}, current = {};
    if (!ParseVersion(job->tag, latest) || !ReadExecutableVersion(current) || latest <= current) return;

    if (ShowAppNotice(window, AppError::UpdateAvailable)) {
        ShellExecuteW(window, L"open", GithubLatestReleaseUrl, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
