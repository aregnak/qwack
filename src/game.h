#pragma once

#include <windows.h>
#include <string>

enum class leagueState
{
    CLOSED,
    LOBBY,
    INGAME
};

inline bool isLeagueFocused()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return false;

    char path[MAX_PATH];
    DWORD size = MAX_PATH;

    bool isLeague = false;
    if (QueryFullProcessImageNameA(hProcess, 0, path, &size))
    {
        std::string exe(path);
        isLeague = exe.find("League of Legends.exe") != std::string::npos;
    }

    CloseHandle(hProcess);
    return isLeague;
}