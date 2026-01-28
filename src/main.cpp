#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#include <SDL.h>
#include <SDL_syswm.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_dx11.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httplib.h"
#include "json.hpp"

// LCU debug prints
#define LCU_LOG(x) std::cout << "[LCU] " << x << std::endl

#include "poll.h"
#include "parser.h"

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
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
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
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                      featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                      &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
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

int main(int, char**)
{
    const int windowW = 150;
    const int windowH = 50;

    // ImGui screen
    ImVec2 imguiSize = ImVec2(120, 30);
    ImVec2 imguiPos = ImVec2((windowW - imguiSize.x) * 0.5f, (windowH - imguiSize.y) * 0.5f);

    // SDL2 init
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    // SDL window flags: borderless, always on top
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS);

    SDL_Window* window = SDL_CreateWindow("CS/min Overlay", 1750, SDL_WINDOWPOS_CENTERED, windowW,
                                          windowH, window_flags);

    if (!window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        return 1;
    }

    // SDL_SetWindowOpacity(window, 0.5f);

    // SDL2: get native HWND using SDL_SysWMinfo
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo))
    {
        SDL_Log("SDL_GetWindowWMInfo failed: %s", SDL_GetError());
        return 1;
    }
    HWND hwnd = wmInfo.info.win.window;

    // Make window click-through (transparent to mouse)
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); // fully opaque but click-through

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    SDL_ShowWindow(window);

    // Init DX11
    if (!InitD3D(hwnd))
    {
        SDL_Log("Failed to init DX11");
        return 1;
    }

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForD3D(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    LCUInfo lcu;
    auto lastPoll = std::chrono::steady_clock::now();
    while (lcu.port == 0)
    {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count() > 500)
        {
            std::cout << "Waiting for League client (open it...)" << std::endl;
            lcu = parseLockfile();
            lastPoll = now;
        }
    }
    poll poller;

    std::string playerName;
    lastPoll = std::chrono::steady_clock::now();
    while (playerName.empty())
    {
        auto now = std::chrono::steady_clock::now();

        // 2 second delaay to let the API start.
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count() > 2000)
        {
            playerName = poller.getPlayerName(lcu);
            lastPoll = now;
        }
    }
    LCU_LOG("Summoner found: " << playerName);

    float cs_per_min = -1.0f;

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPoll).count() > 500)
        {
            if (poller.update())
            {
                cs_per_min = poller.getcs(playerName);
                lastPoll = now;
            }
            else
            {
                cs_per_min = -1.0f;
            }
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Overlay UI
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowPos(imguiPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(imguiSize, ImGuiCond_Always);
        ImGui::Begin("NULL", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

        if (cs_per_min < 0.0f)
        {
            ImGui::Text("Waiting for game.");
        }
        else
        {
            ImGui::Text("CS/min: %.2f", cs_per_min);
        }
        ImGui::End();

        // Render
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    CleanupD3D();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
