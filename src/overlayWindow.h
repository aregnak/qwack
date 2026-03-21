#pragma once

#include "game.h"
#include "window.h"

#include <vector>
#include <string>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

class OverlayWindow : public Window
{
public:
    OverlayWindow(float screenWidth, float screenHeight);

    bool Create() override;

    void renderCspm(float cspm);
    void renderRanks(std::vector<std::string> renderRanks);
    void renderGoldDiff(std::vector<int> itemGoldDiff);

    // Call every frame — handles show/hide based on game state + League focus.
    // Returns true if the overlay is visible after the call.
    bool handleWindowVisibility(leagueState gs);

private:
    // CS/min screen. Dynamic placement, but only tested on 1920x1200.
    ImVec2 cspmSize;
    ImVec2 cspmPos;

    ImVec2 rankSize;
    std::vector<ImVec2> rankPoss;

    ImVec2 itemSumSize;
    std::vector<ImVec2> itemPoss;

    float _screenWidth = 0;
    float _screenHeight = 0;
};