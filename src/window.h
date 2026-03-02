
/* This class handles:
- SDL window pointer
- swapchain
- render target
- common DX11 setup
- cleanup */

#pragma once

#include <SDL.h>
#include <d3d11.h>
// #include "graphicsDevice.h"

class Window
{
public:
    Window();

    virtual ~Window();

    virtual bool Create() = 0; // Override in derrived classes
    virtual void BeginFrame();
    virtual void EndFrame();
    virtual void Cleanup();

    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

    SDL_Window* getWindow();

protected:
    SDL_Window* window = nullptr;
    SDL_WindowID windowID = 0;

    IDXGISwapChain* g_pSwapChain = nullptr;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

    void CreateRenderTarget();
    bool InitD3D(HWND hwnd);
};