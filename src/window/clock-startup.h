#pragma once

#pragma comment(lib, "advapi32.lib")

bool runAtStartup = false;
constexpr int RunAtStartup = 114;
constexpr wchar_t StartupRegistryKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t StartupRegistryValue[] = L"SkeaClock";

bool IsStartupEnabled() {
    DWORD bytes = 0;
    return RegGetValueW(HKEY_CURRENT_USER, StartupRegistryKey, StartupRegistryValue,
        RRF_RT_REG_SZ, nullptr, nullptr, &bytes) == ERROR_SUCCESS && bytes > sizeof(wchar_t);
}

LSTATUS SetStartupEnabled(bool enabled) {
    if (!enabled) {
        const LSTATUS result = RegDeleteKeyValueW(HKEY_CURRENT_USER, StartupRegistryKey, StartupRegistryValue);
        return result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND ? ERROR_SUCCESS : result;
    }
    wchar_t executable[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length) return GetLastError();
    if (length >= MAX_PATH) return ERROR_FILENAME_EXCED_RANGE;
    const std::wstring command = L"\"" + std::wstring(executable, length) + L"\"";
    // Windows Run entries allow command lines of at most 260 characters.
    if (command.size() > MAX_PATH) return ERROR_FILENAME_EXCED_RANGE;
    return RegSetKeyValueW(HKEY_CURRENT_USER, StartupRegistryKey, StartupRegistryValue, REG_SZ,
        command.c_str(), static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
}
