#pragma once

#include <atomic>
#include <string>

#include "game.h"
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
    void renderMenu(SDL_Window* menuWindow, std::atomic<gameState>& gs, const std::string gameMode);

    bool processEvent(const SDL_Event& event);

    void showMenu();

    struct Elements
    {
        bool showCspm = true;
        bool showRanks = true;
        bool showGoldDiff = true;
    };

    Elements elements;

private:
    void handleGeneralTab();
    void handleDebugTab(std::atomic<gameState>& gs, const std::string gameMode);
    void handleAboutTab();

    void handleClosing();
};