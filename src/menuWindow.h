#pragma once

#include "window.h"

#include <SDL.h>
#include <winscard.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

class MenuWindow : public Window
{
public:
    MenuWindow();

    bool Create() override;
    void renderMenu(SDL_Window* menuWindow);

    bool processEvent(const SDL_Event& event);

    void handleVisibility();

    struct Elements
    {
        bool showCspm = true;
        bool showRanks = true;
        bool showGoldDiff = true;
    };

    Elements elements;

private:
    void renderGeneralTab();
    void renderAboutTab();
    // void renderDebugTab();
};