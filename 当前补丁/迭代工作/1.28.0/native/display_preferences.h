#pragma once

#include <windows.h>
#include <string>
#include <cerrno>
#include <cwchar>

struct DisplayPreferences {
    bool english = false, gridLines = false, controlPoints = false;
    bool routePanel = false, scaleBar = true, historyAnimation = false, restoreView = true;
    int gridLevel = 0, opacity = 72, previewRate = 2, historySpeed = 0;
    int uiScale = 0; // 0 follows Windows; 1..4 = 100, 125, 150, 200 percent.
    int panelWidth = 0; // Zero follows the automatic layout.
};

static int readPreferenceInteger(const std::wstring& path, const wchar_t* key, int fallback, int low, int high) {
    wchar_t text[64]{};
    GetPrivateProfileStringW(L"Preferences", key, L"", text, 64, path.c_str());
    if (!text[0]) return fallback;
    wchar_t* end = nullptr;
    errno = 0;
    long value = wcstol(text, &end, 10);
    return errno == ERANGE || end == text || *end || value < low || value > high ? fallback : (int)value;
}

static DisplayPreferences readDisplayPreferences(const std::wstring& path) {
    DisplayPreferences p;
    wchar_t language[32]{};
    GetPrivateProfileStringW(L"Preferences", L"Language", L"zh-CN", language, 32, path.c_str());
    p.english = _wcsicmp(language, L"en-US") == 0;
    p.gridLevel = readPreferenceInteger(path, L"GridLevel", 0, 0, 3);
    p.opacity = readPreferenceInteger(path, L"Opacity", 72, 0, 100);
    p.previewRate = readPreferenceInteger(path, L"PreviewRate", 2, 0, 2);
    p.historySpeed = readPreferenceInteger(path, L"HistorySpeed", 0, 0, 3);
    p.uiScale = readPreferenceInteger(path, L"UiScale", 0, 0, 4);
    p.panelWidth = readPreferenceInteger(path, L"PanelWidth", 0, 0, 520);
    if (p.panelWidth && p.panelWidth < 260) p.panelWidth = 0;
    p.gridLines = readPreferenceInteger(path, L"GridLines", 0, 0, 1) != 0;
    p.controlPoints = readPreferenceInteger(path, L"ControlPoints", 0, 0, 1) != 0;
    p.routePanel = readPreferenceInteger(path, L"RoutePanel", 0, 0, 1) != 0;
    p.scaleBar = readPreferenceInteger(path, L"ScaleBar", 1, 0, 1) != 0;
    p.historyAnimation = readPreferenceInteger(path, L"HistoryAnimation", 0, 0, 1) != 0;
    p.restoreView = readPreferenceInteger(path, L"RestoreView", 1, 0, 1) != 0;
    return p;
}

// Write a complete UTF-16 snapshot beside the destination, flush it, then replace it.
// A failed write never truncates the user's existing settings. Legacy language-only files migrate on save.
static bool writeDisplayPreferences(const std::wstring& path, const DisplayPreferences& p) {
    std::wstring text = L"\xFEFF[Preferences]\r\nSchemaVersion=2\r\nLanguage=";
    text += p.english ? L"en-US\r\n" : L"zh-CN\r\n";
    auto add = [&](const wchar_t* key, int value) { text += std::wstring(key) + L"=" + std::to_wstring(value) + L"\r\n"; };
    add(L"GridLevel",p.gridLevel); add(L"Opacity",p.opacity); add(L"PreviewRate",p.previewRate);
    add(L"HistorySpeed",p.historySpeed); add(L"PanelWidth",p.panelWidth);
    add(L"GridLines",p.gridLines); add(L"ControlPoints",p.controlPoints); add(L"RoutePanel",p.routePanel);
    add(L"ScaleBar",p.scaleBar); add(L"HistoryAnimation",p.historyAnimation);
    add(L"RestoreView",p.restoreView); add(L"UiScale",p.uiScale);
    std::wstring temporary;
    HANDLE file = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 16 && file == INVALID_HANDLE_VALUE; ++attempt) {
        temporary = path + L"." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64()) + L"." + std::to_wstring(attempt) + L".tmp";
        file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD count = 0, size = (DWORD)(text.size() * sizeof(wchar_t));
    bool ok = WriteFile(file, text.data(), size, &count, nullptr) && count == size && FlushFileBuffers(file);
    CloseHandle(file);
    if (ok) ok = MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!ok) DeleteFileW(temporary.c_str());
    // Invalidate the Windows INI cache after an external atomic replacement.
    if (ok) WritePrivateProfileStringW(nullptr, nullptr, nullptr, path.c_str());
    return ok;
}
