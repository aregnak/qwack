#include <iostream>
#include <string>
#include "overlayWindow.h"
#include "log.h"

OverlayWindow::OverlayWindow(float screenWidth, float screenHeight)
    : _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
    , rankPoss(10)
    , itemPoss(5)
{
    cspmSize = ImVec2(120, 30);
    cspmPos = ImVec2((screenWidth - (cspmSize.x * 1.2)), screenHeight / 2.5);

    QWACK_LOG("CS/Min overlay pos: " << cspmPos.x << " " << cspmPos.y);

    rankSize = ImVec2(30, 30);

    // Create rank overlay positions. 10 in total, one for each player.
    // Please don't move the scoreboard in game.
    // Also dynamic placement, but only tested on 1920x1200.
    for (int i = 0; i < rankPoss.size(); i++)
    {
        // Order team ranks
        if (i < 5)
        {
            rankPoss[i] = ImVec2(screenWidth / 5.5f, screenHeight / 3.3f + (i * 80));
        }
        else // Chaos team ranks
        {
            rankPoss[i] = ImVec2(screenWidth / 1.25f, screenHeight / 3.3f + ((i - 5) * 80));
        }
    }

    // Item gold diff overlay positions, 5 in total, one for each lane.
    // Also dynamic placement, but only tested on 1920x1200.
    ImVec2 itemSumSize = ImVec2(50, 30);

    for (int i = 0; i < itemPoss.size(); i++)
    {
        itemPoss[i] =
            ImVec2((screenWidth / 2.0f) - (itemSumSize.x / 2.0f), screenHeight / 3.3f + (i * 80));
    }
}

bool OverlayWindow::Create()
{
    QWACK_LOG("Initializing Overlay Window.");
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS |
                          SDL_WINDOW_TRANSPARENT | SDL_WINDOW_NOT_FOCUSABLE);

    window = SDL_CreateWindow("CS/min Overlay", _screenWidth, _screenHeight, window_flags);

    if (!window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return false;
    }

    windowID = SDL_GetWindowID(window);

    SDL_PropertiesID props = SDL_GetWindowProperties(window);
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

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    QWACK_LOG("Overlay Window Finished Initializing.");
    return true;
}

void OverlayWindow::renderCspm(float cspm)
{
    ImGui::SetNextWindowBgAlpha(0.4f);
    ImGui::SetNextWindowPos(cspmPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(cspmSize, ImGuiCond_Always);

    ImGui::Begin("cspm", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

    if (cspm < 0.0f)
    {
        ImGui::Text("Waiting for game.");
    }
    else
    {
        ImGui::Text("CS/min: %.2f", cspm);
    }
    ImGui::End();
}

void OverlayWindow::renderRanks(std::vector<std::string> renderRanks)
{
    int num = 0;
    for (const auto& pos : rankPoss)
    {
        ImGui::SetNextWindowBgAlpha(0.4f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(rankSize, ImGuiCond_Always);
        ImGui::Begin(("RankedWindow##" + std::to_string(num)).c_str(), nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::Text(renderRanks[num].c_str());
        ImGui::End();
        num++;
    }
}

void OverlayWindow::renderGoldDiff(std::vector<int> itemGoldDiff)
{
    int num = 0;
    for (const auto& pos : itemPoss)
    {
        ImGui::SetNextWindowBgAlpha(0.4f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(itemSumSize, ImGuiCond_Always);
        ImGui::Begin(("ItemWindow##" + std::to_string(num)).c_str(), nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing);

        ImGui::Text("%d", itemGoldDiff[num]);
        ImGui::End();
        num++;
    }
}
