#pragma once

#include "window.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

class OverlayWindow : public Window
{
public:
    OverlayWindow(float screenWidth, float screenHeight);

    bool Create() override;

    void renderCspm(float cspm);

private:
    ImVec2 cspmSize; //= ImVec2(120, 30);
    ImVec2 cspmPos; //= ImVec2((screenWidth - (cspmSize.x * 1.2)), screenHeight / 2.5);

    float _screenWidth = 0;
    float _screenHeight = 0;
};