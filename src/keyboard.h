#pragma once

#include <windows.h>

inline bool IsTabDown()
{
    // VK_TAB is Tab.
    return (GetAsyncKeyState(VK_TAB) & 0x8000) != 0; //
}

inline bool killSwitch()
{
    // VK_NEXT is Page Down.
    return (GetAsyncKeyState(VK_NEXT) & 0x8000) != 0; //
}