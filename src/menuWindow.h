#pragma once

#include "window.h"

#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

class MenuWindow : public Window
{
public:
    MenuWindow();

    bool Create() override;
    void renderMenu(SDL_Window* menuWindow);

private:
};