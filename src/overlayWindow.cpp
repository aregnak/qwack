#include <iostream>
#include "overlayWindow.h"
#include "log.h"

OverlayWindow::OverlayWindow(float screenWidth, float screenHeight)
    : _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
{
    cspmSize = ImVec2(120, 30);
    cspmPos = ImVec2((screenWidth - (cspmSize.x * 1.2)), screenHeight / 2.5);

    QWACK_LOG("CS/Min overlay pos: " << cspmPos.x << " " << cspmPos.y);
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
