#pragma once

#include <SDL.h>
#include "menuWindow.h"

class TrayHelper
{
public:
    TrayHelper(MenuWindow& menu);
    ~TrayHelper();

private:
    static void callback_open(void* userdata, SDL_TrayEntry* invoker);
    static void callback_quit(void* userdata, SDL_TrayEntry* invoker);

    SDL_Tray* tray;
    SDL_TrayMenu* tmenu;
    SDL_TrayEntry* showEntry;
    SDL_TrayEntry* quitEntry;
    SDL_Event e;

    MenuWindow& _menu;

    // void callback_quit(void* userdata, SDL_TrayEntry* invoker);
};