#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <winuser.h>
#include <d3d11.h>
#include <iostream>
#include <string>
#include <chrono>
#include <algorithm>
#include <future>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
// #include <dxgi1_2.h>

#include <SDL.h>
// #include <SDL_syswm.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

#include "httplib.h"
#include "json.hpp"

#include "log.h"
#include "game.h"
#include "gamePoller.h"
#include "poll.h"
#include "parser.h"
#include "lcuClient.h"
#include "playerInfo.h"
#include "keyboard.h"

// DX11 & SDL window
#include "window.h"
#include "overlayWindow.h"
#include "menuWindow.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

int main(int, char**)
{
    // SDL2 init
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Get screen area without the taskbar.
    float screenWidth = 0; //workArea.right - workArea.left;
    float screenHeight = 0; //workArea.bottom - workArea.top;

    {
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        screenWidth = workArea.right - workArea.left;
        screenHeight = workArea.bottom - workArea.top;
        QWACK_LOG("Primary screen work area: " << screenWidth << "x" << screenHeight);
    }

    OverlayWindow overlay(screenWidth, screenHeight);
    overlay.Create();
    SDL_Window* overlayWindow = overlay.getWindow();

    MenuWindow menu;
    menu.Create();
    SDL_Window* menuWindow = menu.getWindow();

    // !!!!!
    // TODO: Right now, there is a device and device context for each window. Merge them together.
    // ImGui overlay window context
    ImGuiContext* overlayCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(overlayCtx);
    ImGui_ImplSDL3_InitForD3D(overlayWindow);
    ImGui_ImplDX11_Init(overlay.g_pd3dDevice, overlay.g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    // Menu context
    ImGuiContext* menuCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(menuCtx);
    ImGui_ImplSDL3_InitForD3D(menuWindow);
    ImGui_ImplDX11_Init(menu.g_pd3dDevice, menu.g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    // Finish all DX11, SDL, and ImGui setup.

    // Friendly welcome message!
    QWACK_LOG("Thank you for choosing (or being forced to) try my program, Enjoy!");

    std::atomic<bool> running = true;
    std::atomic<bool> practicetool = false;

    std::atomic<float> csPerMin = -1.0f;

    // Initial game state of closed (league not open).
    std::atomic<gameState> gameState = gameState::CLOSED;

    GamePoller gp;

    // Polling thread.
    std::thread lcuThread(
        [&]()
        {
            auto lastPoll = std::chrono::steady_clock::now();

            LCUClient lcuC;

            try
            {
                while (running.load())
                {
                    if (!std::filesystem::exists("C:\\Riot Games\\League of Legends\\lockfile"))
                    {
                        if (gameState.load() != gameState::CLOSED)
                        {
                            gameState.store(gameState::CLOSED);

                            // reset playerName & print message.
                            gp.resetPlayerName();

                            LCU_LOG("Lockfile not found. League is closed (keep it closed pls).");
                        }

                        // ! Change this name.
                        gp.setPrintedWaitingForClient(false);
                    }

                    // Game state management.
                    switch (gameState.load())
                    {
                        case gameState::CLOSED:
                            gp.handleClosedState(lcuC, gameState, running);
                            break;

                        case gameState::LOBBY:
                            gp.handleLobbyState(lcuC, gameState, practicetool, csPerMin);
                            break;

                        case gameState::INGAME:
                            gp.handleInGameState(lcuC, csPerMin, gameState, practicetool);
                            break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
            catch (const std::exception& e)
            {
                LCU_LOG("[THREAD EXCEPTION] " << e.what());
                running.store(false);
            }
            catch (...)
            {
                LCU_LOG("[UNKNOWN THREAD EXCEPTION]");
                running.store(false);
            }
        });

    // Main thread.
    SDL_Event event;
    bool gotRanks = false;

    std::vector<int> itemGoldDiff;

    std::vector<std::string> renderRanks;

    while (running.load())
    {
        while (SDL_PollEvent(&event))
        {
            ImGui::SetCurrentContext(menuCtx);
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                running.store(false);
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(menuWindow))
            {
                running.store(false);
            }

            ImGui::SetCurrentContext(overlayCtx);
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(overlayWindow))
            {
                running.store(false);
            }
        }

        // CTRL+C doesn't really work, I think because the window is unfocusable.
        if (killSwitch())
        {
            QWACK_LOG("Killswitch activated. Exiting.");
            running.store(false);
        }

        // Render menu window
        ImGui::SetCurrentContext(menuCtx);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        menu.renderMenu(menuWindow);

        ImGui::Render();
        menu.BeginFrame();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        menu.EndFrame();

        // Check if League is focused, set visible.
        overlay.handleWindowVisibility(gameState.load());

        // Focus checking and key checking.
        if (gameState.load() == gameState::INGAME)
        {
            if (!gotRanks && gp.isRanksReady())
            {
                renderRanks = gp.getRanks();
                gotRanks = true;
            }

            if (gp.isItemDiffReady())
            {
                itemGoldDiff = gp.getItemGoldDiff();
            }

            ImGui::SetCurrentContext(overlayCtx);
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Overlays are down here.
            // CS/Min overlay

            if (overlay.isVisible())
            {
                overlay.renderCspm(csPerMin.load());
            }

            // Ranks & item gold diff overlay
            // Check if pressing tab (scoreboard).
            if (!practicetool.load())
            {
                if (overlay.isVisible() && IsTabDown())
                {
                    overlay.renderRanks(renderRanks);
                    overlay.renderGoldDiff(itemGoldDiff);
                }
            }

            ImGui::Render();
            overlay.BeginFrame();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            overlay.EndFrame();
        }
        else
        {
            if (gotRanks)
            {
                gotRanks = false;
            }
        }

        // Limit to 30 fps.
        SDL_Delay(33);
    }

    // Cleanup
    running.store(false);

    if (lcuThread.joinable())
    {
        lcuThread.join();
    }

    overlay.Cleanup();
    SDL_DestroyWindow(overlayWindow);

    ImGui::SetCurrentContext(overlayCtx);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(overlayCtx);

    menu.Cleanup();
    SDL_DestroyWindow(menuWindow);

    ImGui::SetCurrentContext(menuCtx);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(menuCtx);

    SDL_Quit();

    return 0;
}