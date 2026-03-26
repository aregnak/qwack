#pragma once

#include <SDL3/SDL.h>

#include "menuWindow.h"
#include "trayIconIco.h"

class TrayHelper
{
public:
    TrayHelper(MenuWindow& menu);
    ~TrayHelper();

private:
    SDL_Surface* loadTrayIcon();

    static void callback_open(void* userdata, SDL_TrayEntry* invoker);
    static void callback_quit(void* userdata, SDL_TrayEntry* invoker);

    SDL_Tray* tray = nullptr;
    SDL_TrayMenu* tmenu = nullptr;
    SDL_TrayEntry* showEntry = nullptr;
    SDL_TrayEntry* quitEntry = nullptr;
    SDL_Surface* trayIconSurface = nullptr;

    SDL_Event e;

    MenuWindow& _menu;

    // void callback_quit(void* userdata, SDL_TrayEntry* invoker);
};