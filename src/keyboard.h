#pragma once

#include <windows.h>

inline bool IsTabDown()
{
    // VK_TAB is Tab.
    return (GetAsyncKeyState(VK_TAB) & 0x8000) != 0; //
}

inline bool isHomeDown()
{
    // VK_HOME is Home key.
    return (GetAsyncKeyState(VK_HOME) & 0x8000) != 0; //
}

inline bool killSwitch()
{
    // VK_NEXT is Page Down.
    return (GetAsyncKeyState(VK_NEXT) & 0x8000) != 0; //
}