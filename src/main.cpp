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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Helper: create DX11 render target
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

// Helper: cleanup DX11 render target
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// DX11 initialization
bool InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;

// Use VS debug directive for now.
#ifdef _DEBUG
    // DX11 Graphics debug flags.
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //DEBUG

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res ==
        DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                                            createDeviceFlags, featureLevelArray, 2,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
                                            &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

// DX11 cleanup
void CleanupD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

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

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS |
                          SDL_WINDOW_TRANSPARENT | SDL_WINDOW_NOT_FOCUSABLE);

    SDL_Window* window =
        SDL_CreateWindow("CS/min Overlay", screenWidth, screenHeight, window_flags);

    SDL_SetWindowSize(window, screenWidth, screenHeight);

    if (!window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return 1;
    }

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
        return 1;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_ShowWindow(window);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForD3D(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    // Finish all DX11, SDL, and ImGui setup.

    // Friendly welcome message!
    QWACK_LOG("Thank you for choosing (or being forced to) try my program, Enjoy!");

    // CS/min screen. Static until I add dynamic placement.
    ImVec2 cspmSize = ImVec2(120, 30);
    ImVec2 cspmPos = ImVec2((screenWidth - (cspmSize.x * 1.2)), screenHeight / 2.5);
    QWACK_LOG("CS/Min overlay pos: " << cspmPos.x << " " << cspmPos.y);

    ImVec2 rankSize = ImVec2(30, 30);
    std::vector<ImVec2> rankPoss(10);

    // Create rank overlay positions. 10 in total, one for each player.
    // Please don't move the scoreboard in game.
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

    ImVec2 itemSumSize = ImVec2(70, 30);
    std::vector<ImVec2> itemPoss(5);

    for (int i = 0; i < itemPoss.size(); i++)
    {
        // Order team ranks
        // Screen width: centered, screen height: every whatever amount is down there, it works.
        itemPoss[i] =
            ImVec2((screenWidth / 2.0f) - (itemSumSize.x / 2.0f), screenHeight / 3.3f + (i * 80));
    }

    std::vector<std::string> ranks;
    std::vector<PlayerInfo> players(10);
    std::vector<int> itemGoldDiff(5);
    std::mutex dataMutex;

    std::atomic<bool> running = true;
    std::atomic<bool> practicetool = false;
    std::atomic<bool> playersLoaded = false;

    std::atomic<float> csPerMin = -1.0f;
    // std::atomic<float> currentGold = 500.0f;
    // std::atomic<float> gameTime = 0.0f;

    // Initial game state of closed (league not open).
    std::atomic<gameState> gameState = gameState::CLOSED;

    // Polling thread.
    std::thread lcuThread(
        [&]()
        {
            auto lastPoll = std::chrono::steady_clock::now();

            poll poller;
            GamePoller gp;

            LCUClient lcuC;
            std::string playerName;

            bool printedWaitingForClient = false;

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
                            playerName = std::string();

                            LCU_LOG("Lockfile not found. League is closed (keep it closed pls).");
                        }
                        printedWaitingForClient = false;
                    }

                    // Game state management.
                    switch (gameState.load())
                    {
                        case gameState::CLOSED:
                            gp.handleClosedState(lcuC, poller, gameState, running, playerName,
                                                 printedWaitingForClient);
                            break;

                        case gameState::LOBBY:
                            gp.handleLobbyState(gameState, poller, ranks, players, playersLoaded,
                                                practicetool, csPerMin);
                            break;

                        case gameState::INGAME:
                            gp.handleInGameState(lcuC, players, ranks, poller, csPerMin, gameState,
                                                 playersLoaded, practicetool, playerName,
                                                 dataMutex);
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
    bool inLobby = true;
    bool windowHidden = false;
    bool tabDown = false;

    while (running.load())
    {
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                running.store(false);
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
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

        // Main game state logic.
        // Lobby state (Client open, not in game)
        // TODO: throw all this into game poller!!!!!!!!!!!
        // if (gameState.load() == gameState::LOBBY)
        // {
        //     if (!inLobby)
        //     {
        //         inLobby = true;
        //     }
        // }
        // else if (gameState.load() == gameState::INGAME)
        // {
        //     if (inLobby)
        //     {
        //         inLobby = false;

        //         QWACK_LOG("GLHF.");
        //     }
        // }
        // else if (gameState.load() == gameState::CLOSED)
        // {
        //     inLobby = false;
        // }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // TODO: Main settings overlay
        {
            // ImGui::SetNextWindowBgAlpha(0.4f);
            // ImGui::SetNextWindowPos(ImVec2(screenWidth / 2, screenHeight / 2), ImGuiCond_Always);
            // ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Always);

            // ImGui::Begin("Test", nullptr);

            // ImGui::Text("Hello from another window!");
            // ImGui::End();
        }

        // Focus checking and key checking.
        if (gameState.load() == gameState::INGAME)
        {
            if (isLeagueFocused())
            {
                if (windowHidden)
                {
                    SDL_ShowWindow(window);
                    windowHidden = false;
                }
            }
            else
            {
                if (!windowHidden)
                {
                    SDL_HideWindow(window);
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

                    ImGui::Text(ranks[num].c_str());
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
        }

        // Render
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // With vsync

        // Limit to 30 fps.
        SDL_Delay(33);
    }

    // Cleanup
    running.store(false);
    if (lcuThread.joinable())
    {
        lcuThread.join();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    CleanupD3D();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}