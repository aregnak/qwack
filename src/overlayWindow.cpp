#include "overlayWindow.h"

OverlayWindow::OverlayWindow(float screenWidth, float screenHeight)
    : _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
{
}

bool OverlayWindow::Create()
{
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS |
                          SDL_WINDOW_TRANSPARENT | SDL_WINDOW_NOT_FOCUSABLE);

    _window = SDL_CreateWindow("CS/min Overlay", _screenWidth, _screenHeight, window_flags);

    if (!_window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return false;
    }

    SDL_PropertiesID props = SDL_GetWindowProperties(_window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); // fully opaque but click-through

    // Init DX11
    if (!InitD3D(hwnd))
    {
        SDL_Log("Failed to init DX11");
        return false;
    }

    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(_window);

    return true;
}

SDL_Window* OverlayWindow::getWindow()
{
    return _window;
    //
}