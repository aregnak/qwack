#include <iostream>
#include "menuWindow.h"
#include "game.h"
#include "keyboard.h"
#include "log.h"
#include "version.h"

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

    visible = true;
    return true;
}

void MenuWindow::renderMenu(SDL_Window* menuWindow, std::atomic<gameState>& gs)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(menuWindow, &w, &h);
    ImGui::SetNextWindowSize(ImVec2((float)w, (float)h), ImGuiCond_Always);

    ImGui::Begin("Settings", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Qwack Settings");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("SettingsTabs"))
    {
        handleGeneralTab();

        handleDebugTab(gs);

        handleAboutTab();

        ImGui::EndTabBar();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    handleClosing();

    ImGui::End();
}

bool MenuWindow::processEvent(const SDL_Event& event)
{
    // Only handle events for this window.
    bool isOurs = false;
    switch (event.type)
    {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_EXPOSED:
            isOurs = (event.window.windowID == windowID);
            break;

        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
            isOurs = (event.motion.windowID == windowID);
            break;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            isOurs = (event.key.windowID == windowID);
            break;

        case SDL_EVENT_TEXT_INPUT:
            isOurs = (event.text.windowID == windowID);
            break;

        default:
            return false;
    }

    if (!isOurs)
        return false;

    // Handle window close -> hide instead of quitting the app.
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == windowID)
    {
        QWACK_LOG("Menu hidden using close request.");
        setVisibility(false);
        return true;
    }
}

void MenuWindow::handleVisibility()
{
    if (isHomeDown())
    {
        if (!visible)
        {
            QWACK_LOG("Menu shown using Home key.");
            setVisibility(true);
        }
    }
}

// Private
void MenuWindow::handleGeneralTab()
{
    if (ImGui::BeginTabItem("General"))
    {
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
            ImGui::Text("Show this menu: Home key");
            ImGui::Text("Kill Switch: Page Down");
            ImGui::Text("(Tab shows scoreboard overlays in game)");
        }

        ImGui::EndTabItem();
    }
}

void MenuWindow::handleDebugTab(std::atomic<gameState>& gs)
{
    ImGui::Spacing();

    if (ImGui::BeginTabItem("Debug"))
    {
        switch (gs.load())
        {
            case gameState::CLOSED:
                _debugState = "[CLOSED] Launcher closed.";
                break;

            case gameState::LOBBY:
                _debugState = "[LOBBY] Launcher open, in lobby.";
                break;

            case gameState::INGAME:
                _debugState = "[INGAME] In game.";
                break;

            default:
                _debugState = "Unknown state.";
                break;
        }

        ImGui::Text("State: %s", _debugState.c_str());

        ImGui::EndTabItem();
    }
}

void MenuWindow::handleAboutTab()
{
    if (ImGui::BeginTabItem("About"))
    {
        ImGui::Text("Qwack - League of Legends Overlay");
        ImGui::Text("Version %s", APP_VERSION_STRING);

        ImGui::EndTabItem();
    }
}

void MenuWindow::handleClosing()
{
    if (ImGui::Button("Close Qwack"))
    {
        ImGui::OpenPopup("Confirm Close");
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal("Confirm Close", NULL, flags))
    {
        ImGui::Text("Are you sure you want to close?");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            QWACK_LOG("Program terminated through menu close.");

            SDL_Event event;
            event.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&event);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}