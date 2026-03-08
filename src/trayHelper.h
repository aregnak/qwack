#pragma once

#include <SDL.h>

class TrayHelper
{
public:
    TrayHelper();
    ~TrayHelper();

private:
    SDL_Tray* tray;
    SDL_TrayMenu* tmenu;
    SDL_TrayEntry* entry;
    SDL_Event e;

    // void callback_quit(void* userdata, SDL_TrayEntry* invoker);
};