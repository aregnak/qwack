#pragma once

#include <windows.h>
#include <string>
#include <tlhelp32.h>

enum class gameState
{
    CLOSED,
    LOBBY,
    INGAME
};

// Finds the main window of "League of Legends.exe" by enumerating windows once.
// Call this when entering INGAME state and cache the result.
inline HWND findLeagueWindow()
{
    HWND result = nullptr;

    EnumWindows(
        [](HWND hwnd, LPARAM lParam) -> BOOL
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);

            HANDLE hProcess =
                OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!hProcess)
                return TRUE; // continue

            char path[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameA(hProcess, 0, path, &size))
            {
                std::string exe(path);
                if (exe.find("League of Legends.exe") != std::string::npos)
                {
                    CloseHandle(hProcess);
                    *reinterpret_cast<HWND*>(lParam) = hwnd;
                    return FALSE; // found it, stop enumerating
                }
            }

            CloseHandle(hProcess);
            return TRUE; // continue
        },
        reinterpret_cast<LPARAM>(&result));

    return result;
}

// Cheap per-frame check: just compare the foreground window handle.
inline bool isLeagueFocused(HWND leagueHwnd)
{
    return leagueHwnd && GetForegroundWindow() == leagueHwnd;
}