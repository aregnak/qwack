#include <iostream>
#include "menuWindow.h"
#include "log.h"

MenuWindow::MenuWindow() {}

bool MenuWindow::Create()
{
    QWACK_LOG("Initializing Menu Window.");

    window = SDL_CreateWindow("Qwack", 600, 450, 0);

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

void MenuWindow::renderMenu(SDL_Window* menuWindow)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    int w, h;
    SDL_GetWindowSize(menuWindow, &w, &h);
    ImGui::SetNextWindowSize(ImVec2((float)w, (float)h), ImGuiCond_Always);

    ImGui::Begin("Settings", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Qwack Settings");
    ImGui::Separator();
    ImGui::Spacing();

    // Overlay toggles
    if (ImGui::CollapsingHeader("Overlays", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Show CS/min Overlay", &elements.showCspm);
        ImGui::Checkbox("Show Rank Overlay", &elements.showRanks);
        ImGui::Checkbox("Show Item Gold Diff Overlay", &elements.showGoldDiff);
        ImGui::Spacing();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Hotkeys"))
    {
        ImGui::Text("Kill Switch: Page Down");
        ImGui::Text("(Tab shows scoreboard overlays in game)");
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("About"))
    {
        ImGui::Text("Qwack - League of Legends Overlay");
    }

    ImGui::End();
}
