#pragma once

#include "window.h"

class OverlayWindow : public Window
{
public:
    OverlayWindow(float screenWidth, float screenHeight);

    bool Create() override;
    SDL_Window* getWindow();

private:
    SDL_Window* _window;

    float _screenWidth = 0;
    float _screenHeight = 0;
};