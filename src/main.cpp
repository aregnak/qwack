#include <mutex>
#include <thread>
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
// #include <dxgi1_2.h>

#include <SDL.h>
// #include <SDL_syswm.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"
#include "json.hpp"

#include "debugPrints.h"
#include "game.h"
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
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
    NEWLINE;
    QWACK_LOG("Thank you for choosing (or being forced to) try my program, Enjoy!");
    NEWLINE;

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

    std::atomic<float> csPerMin = -1.0f;
    std::atomic<float> currentGold = 500.0f;
    std::atomic<float> gameTime = 0.0f;
    std::atomic<bool> running = true;
    std::atomic<bool> practicetool = false;
    std::atomic<bool> playersLoaded = false;

    // Initial game state of closed (league not open).
    std::atomic<gameState> gameState = gameState::CLOSED;

    // Polling thread.
    std::thread lcuThread(
        [&]()
        {
            auto lastPoll = std::chrono::steady_clock::now();

            poll poller;

            LCUClient lcuC;
            std::string playerName;

            int currentCS = 0;
            int lastCS = 0;
            int estimatedCS = 0;
            int totalCS = 0;
            float lastGold = 500.0f;

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
                            playerName = std::string();

                            NEWLINE;
                            LCU_LOG("Lockfile not found. League is closed (keep it closed pls).");
                        }

                        printedWaitingForClient = false;
                    }

                    // Lobby state is currently handled in main thread.

                    // League client is closed.
                    if (gameState.load() == gameState::CLOSED)
                    {
                        LCUInfo lcu;

                        while (running.load() && lcu.port == 0)
                        {
                            if (!printedWaitingForClient)
                            {
                                LCU_LOG("Waiting for League client (open it...)");
                                printedWaitingForClient = true;
                            }

                            lcu = parseLockfile();

                            if (lcu.port == 0)
                            {
                                std::this_thread::sleep_for(std::chrono::seconds(10));
                            }
                        }

                        lcuC.connect(lcu);

                        while (running.load() && playerName.empty())
                        {
                            playerName = poller.getCurrentSummoner(lcuC);

                            if (playerName.empty())
                            {
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                            }
                            else
                            {
                                LCU_LOG("Summoner found: " << playerName);

                                gameState.store(gameState::LOBBY);
                            }
                        }
                    }

                    // State is in game (INGAME).
                    if (poller.update())
                    {
                        gameState.store(gameState::INGAME);

                        if (!playersLoaded.load())
                        {
                            std::vector<PlayerInfo> newPlayers(10);
                            std::vector<std::string> newRanks;
                            LCU_LOG("Polling Player Info...");

                            poller.getSessionInfo(lcuC, newPlayers);

                            if (newPlayers.empty())
                            {
                                practicetool.store(true);
                                QWACK_LOG("Gamemode is practice tool, skipping player info");
                            }

                            if (!practicetool.load())
                            {
                                // Unfortunately my understanding of the LCU API led me here,
                                // to get players' ranks, we need the puuid, but to get their in game
                                // stats, we need the live API (yes, different).
                                // this code is very messy for now.

                                for (auto& p : newPlayers)
                                {
                                    p.riotID = poller.getPlayerName(lcuC, p.puuid);
                                    p.rank = poller.getPlayerRank(lcuC, p.puuid);

                                    p.champ = poller.getChampionNameById(p.champID);
                                    poller.getPlayerRoleAndTeam(p);
                                    // std::cout << "puuid: " << p.puuid << " champId: " << p.champID
                                    //           << " riotID: " << p.riotID << " rank: " << p.rank
                                    //           << " role: " << p.role << " team: " << p.team
                                    //           << std::endl;
                                }

                                sortPlayers(newPlayers);

                                for (const auto& p : newPlayers)
                                {
                                    char rankLetter = p.rank[0];
                                    int tierNumber =
                                        romanToInt(p.rank.substr(p.rank.find(' ') + 1));

                                    if (tierNumber != -1)
                                    {
                                        std::ostringstream oss;
                                        oss << rankLetter << tierNumber;
                                        newRanks.push_back(oss.str());
                                    }
                                    else
                                    {
                                        newRanks.push_back("");
                                    }

                                    // std::cout << "puuid: " << p.puuid << " riotID: " << p.riotID
                                    //           << " rank: " << p.rank << " role: " << p.role
                                    //           << " team: " << p.team << std::endl;
                                }
                                QWACK_LOG("Successfully loaded players.");
                                practicetool.store(false);
                            }
                            playersLoaded.store(true);

                            std::lock_guard<std::mutex> lock(dataMutex);

                            players = std::move(newPlayers);
                            ranks = std::move(newRanks);
                        }

                        currentCS = poller.getcs(playerName);
                        float gold = poller.getGold();
                        float time = poller.getGameTime();

                        // Minons spawn after 30 seconds, no need to measure anything before that.
                        if (time >= 30.0f)
                        {
                            // CS counter updates every 10 CS, this algorithm will help estimate through gold delta.
                            if (lastCS == currentCS)
                            {
                                if (gold - lastGold > 14.0f)
                                {
                                    estimatedCS++;
                                }
                                // Get gold difference twice per second, we only want the delta IF there is a change of 14 or higher during poll.
                                lastGold = gold;
                            }
                            else
                            {
                                lastCS = currentCS;
                                estimatedCS = 0;
                            }

                            totalCS = estimatedCS + currentCS;

                            // This is really just to make it a slight bit more accurate in case
                            // something triggers a lot of additional "cs" but in reality it is something else.
                            if (totalCS - currentCS > 10)
                            {
                                estimatedCS--;
                            }
                            csPerMin.store(totalCS / (time / 60.0f), std::memory_order_relaxed);
                        }
                        // The cs/min counter will always be an approximation because the API updates the number
                        // every 10 cs, this algorithm will somewhat smoothen that out, but any

                        currentGold.store(gold, std::memory_order_relaxed);
                        gameTime.store(time, std::memory_order_relaxed);

                        // Item price polling
                        if (!practicetool)
                        {
                            auto now = std::chrono::steady_clock::now();

                            std::lock_guard<std::mutex> lock(dataMutex);

                            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPoll)
                                    .count() > 2)
                            {
                                for (size_t i = 0; i < players.size() / 2; i++)
                                {
                                    PlayerInfo& currentPlayer = players[i];
                                    PlayerInfo& laneOpponent = players[i + 5];

                                    poller.getPlayerItemSum(currentPlayer);
                                    poller.getPlayerItemSum(laneOpponent);

                                    itemGoldDiff[i] =
                                        (currentPlayer.itemsPrice - laneOpponent.itemsPrice);
                                }
                                lastPoll = now;
                            }
                        }
                    }
                    else
                    {
                        gameState.store(gameState::LOBBY);
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
        float csDisplay = csPerMin.load();
        float goldDisplay = currentGold.load();
        float timeDisplay = gameTime.load();

        // CTRL+C doesn't really work, I think because the window is unfocusable.
        if (killSwitch())
        {
            QWACK_LOG("Killswitch activated. Exiting.");
            running.store(false);
        }

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

        // Lobby state (Client open, not in game)
        if (gameState.load() == gameState::LOBBY)
        {
            if (!inLobby)
            {
                inLobby = true;

                ranks.clear();
                players = std::vector<PlayerInfo>(10);

                playersLoaded.store(false);
                practicetool.store(false);
                csPerMin.store(0.0f);

                QWACK_LOG("In lobby. Waiting for game.");
            }
        }
        else if (gameState.load() == gameState::INGAME)
        {
            if (inLobby)
            {
                inLobby = false;

                QWACK_LOG("GLHF.");
            }
        }
        else if (gameState.load() == gameState::CLOSED)
        {
            inLobby = false;
        }

        // std::cout << "\n" << std::endl;
        // LCU_LOG("Gold delta: " << currentGold - lastGold);
        // LCU_LOG("Last Gold: " << lastGold);
        // LCU_LOG("Current Gold: " << currentGold);
        // std::cout << "\n" << std::endl;

        // LCU_LOG("Total CS: " << totalCS);
        // LCU_LOG("Last CS: " << lastCS);
        // LCU_LOG("CS/min: " << csPerMin);

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

        ImGui::SetNextWindowBgAlpha(0.4f);
        // CS/Min overlay
        {
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
                ImGui::Text("CS/min: %.2f", csDisplay);
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