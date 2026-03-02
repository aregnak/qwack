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

bool isLeagueFocused()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return false;

    char path[MAX_PATH];
    DWORD size = MAX_PATH;

    bool isLeague = false;
    if (QueryFullProcessImageNameA(hProcess, 0, path, &size))
    {
        std::string exe(path);
        isLeague = exe.find("League of Legends.exe") != std::string::npos;
    }

    CloseHandle(hProcess);
    return isLeague;
}

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

    OverlayWindow overlayWindow(screenWidth, screenHeight);
    overlayWindow.Create();
    SDL_Window* overlay = overlayWindow.getWindow();

    MenuWindow menuWindow;
    menuWindow.Create();
    SDL_Window* menu = menuWindow.getWindow();

    // !!!!!
    // TODO: Right now, there is a device and device context for each window. Merge them together.
    // ImGui overlay window context
    ImGuiContext* overlayCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(overlayCtx);
    ImGui_ImplSDL3_InitForD3D(overlay);
    ImGui_ImplDX11_Init(overlayWindow.g_pd3dDevice, overlayWindow.g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    // Menu context
    ImGuiContext* menuCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(menuCtx);
    ImGui_ImplSDL3_InitForD3D(menu);
    ImGui_ImplDX11_Init(menuWindow.g_pd3dDevice, menuWindow.g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    // Finish all DX11, SDL, and ImGui setup.

    // Friendly welcome message!
    QWACK_LOG("Thank you for choosing (or being forced to) try my program, Enjoy!");

    // CS/min screen. Dynamic placement, but only tested on 1920x1200.
    ImVec2 cspmSize = ImVec2(120, 30);
    ImVec2 cspmPos = ImVec2((screenWidth - (cspmSize.x * 1.2)), screenHeight / 2.5);
    QWACK_LOG("CS/Min overlay pos: " << cspmPos.x << " " << cspmPos.y);

    ImVec2 rankSize = ImVec2(30, 30);
    std::vector<ImVec2> rankPoss(10);

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
    std::vector<ImVec2> itemPoss(5);

    for (int i = 0; i < itemPoss.size(); i++)
    {
        itemPoss[i] =
            ImVec2((screenWidth / 2.0f) - (itemSumSize.x / 2.0f), screenHeight / 3.3f + (i * 80));
    }

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
    bool windowHidden = false;
    bool tabDown = false;
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
                event.window.windowID == SDL_GetWindowID(menu))
            {
                running.store(false);
            }

            ImGui::SetCurrentContext(overlayCtx);
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(overlay))
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
        ImGui::SetCurrentContext(menuCtx);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Menu window
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            int w, h;
            SDL_GetWindowSize(menu, &w, &h);
            ImGui::SetNextWindowSize(ImVec2((float)w, (float)h), ImGuiCond_Always);

            ImGui::Begin("Settings", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Qwack Settings");
            ImGui::Separator();
            ImGui::Spacing();

            // ! Placeholder, just for testing.
            static bool x = false;
            static bool y = false;
            static bool z = true;
            // Overlay toggles
            if (ImGui::CollapsingHeader("Overlays", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show CS/min Overlay", &x);
                ImGui::Checkbox("Show Rank Overlay", &y);
                ImGui::Checkbox("Show Item Gold Diff Overlay", &z);
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

        ImGui::Render();
        menuWindow.BeginFrame();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        menuWindow.EndFrame();

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

            if (isLeagueFocused())
            {
                if (windowHidden)
                {
                    SDL_ShowWindow(overlay);
                    windowHidden = false;
                }
            }
            else
            {
                if (!windowHidden)
                {
                    SDL_HideWindow(overlay);
                    windowHidden = true;
                }
            }

            // Check if pressing tab (scoreboard).
            if (!practicetool.load())
            {
                if (!windowHidden && IsTabDown())
                {
                    tabDown = true;
                }
                else
                {
                    tabDown = false;
                }
            }

            ImGui::SetCurrentContext(overlayCtx);
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Overlays are down here.
            // CS/Min overlay
            if (!windowHidden)
            {
                ImGui::SetNextWindowBgAlpha(0.4f);
                ImGui::SetNextWindowPos(cspmPos, ImGuiCond_Always);
                ImGui::SetNextWindowSize(cspmSize, ImGuiCond_Always);

                ImGui::Begin("cspm", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoFocusOnAppearing);

                if (csPerMin.load() < 0.0f)
                {
                    ImGui::Text("Waiting for game.");
                }
                else
                {
                    ImGui::Text("CS/min: %.2f", csPerMin.load());
                }
                ImGui::End();
            }

            // Ranks & item gold diff overlay
            if (tabDown)
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

                num = 0;
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

            ImGui::Render();
            overlayWindow.BeginFrame();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            overlayWindow.EndFrame();
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

    overlayWindow.Cleanup();
    SDL_DestroyWindow(overlay);

    ImGui::SetCurrentContext(overlayCtx);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(overlayCtx);

    menuWindow.Cleanup();
    SDL_DestroyWindow(menu);

    ImGui::SetCurrentContext(menuCtx);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(menuCtx);

    SDL_Quit();

    return 0;
}