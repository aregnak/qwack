#pragma once

#include <windows.h>

inline bool IsTabDown()
{
    return (GetAsyncKeyState(VK_TAB) & 0x8000) != 0; //
}