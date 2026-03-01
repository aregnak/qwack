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

    if (!window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return false;
    }

    return true;
}

SDL_Window* OverlayWindow::getWindow()
{
    return _window;
    //
}