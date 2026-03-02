#include <iostream>
#include "menuWindow.h"
#include "log.h"

MenuWindow::MenuWindow() {}

bool MenuWindow::Create()
{
    QWACK_LOG("Initializing Menu Window.");

    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE);

    window = SDL_CreateWindow("Qwack", 600, 450, flags);

    if (!window)
    {
        SDL_Log("Menu SDL_CreateWindow Error: %s", SDL_GetError());
        return false;
    }

    windowID = SDL_GetWindowID(window);

    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

    // Init DX11
    if (!InitD3D(hwnd))
    {
        SDL_Log("Failed to init DX11");
        return false;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    QWACK_LOG("Menu Window Finished Initializing.");
    return true;
}