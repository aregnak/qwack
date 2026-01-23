#include <windows.h>
#include <d3d11.h>
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_sdl3.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*           g_pSwapChain = nullptr;
static ID3D11RenderTargetView*   g_mainRenderTargetView = nullptr;

// Global dummy CS/min counter
float cs_per_min = 0.0f;

int main(int argc, char* argv[])
{
    // SDL Init
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN;

    SDL_Window* window = SDL_CreateWindow("CS/min Overlay", 300, 50, window_flags);

    // DX11 + ImGui Init (pseudo-code)
    // Initialize your D3D device + ImGui SDL3 backend
    ImGui_ImplSDL3_InitForD3D(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool running = true;
    SDL_Event event;

    while (running)
    {
        // Event loop
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Draw overlay
        ImGui::Begin("CS/min Overlay");
        ImGui::Text("CS/min: %.2f", cs_per_min);
        ImGui::End();

        // Render
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Dummy update
        cs_per_min += 0.1f; // increments every frame
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
