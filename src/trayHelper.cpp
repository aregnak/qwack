#include "trayHelper.h"

#include <SDL3_image/SDL_image.h>

#include "log.h"
#include "menuWindow.h"

TrayHelper::TrayHelper(MenuWindow& menu)
    : _menu(menu)
{
    trayIconSurface = loadTrayIcon();
    tray = SDL_CreateTray(trayIconSurface, "Qwack");
    if (!tray)
    {
        QWACK_LOG("SDL_CreateTray failed: " << SDL_GetError());
        return;
    }

    // Create a context menu for the tray.
    tmenu = SDL_CreateTrayMenu(tray);

    // Create a button in the context menu.
    showEntry = SDL_InsertTrayEntryAt(tmenu, -1, "Open Menu", SDL_TRAYENTRY_BUTTON);
    quitEntry = SDL_InsertTrayEntryAt(tmenu, -1, "Quit", SDL_TRAYENTRY_BUTTON);

    // Set the callback for the button
    SDL_SetTrayEntryCallback(showEntry, callback_open, this);
    SDL_SetTrayEntryCallback(quitEntry, callback_quit, this);
}

TrayHelper::~TrayHelper()
{
    SDL_DestroyTray(tray);
    //
}

SDL_Surface* TrayHelper::loadTrayIcon()
{
    SDL_IOStream* io = SDL_IOFromConstMem(qwack_ico, qwack_ico_len);
    if (!io)
    {
        QWACK_LOG("SDL_IOFromConstMem failed: " << SDL_GetError());
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load_IO(io, 1);
    if (!surface)
    {
        QWACK_LOG("IMG_Load_IO failed: " << SDL_GetError());
        return nullptr;
    }

    return surface;
}

void TrayHelper::callback_open(void* userdata, SDL_TrayEntry* invoker)
{
    TrayHelper* self = static_cast<TrayHelper*>(userdata);
    self->_menu.showMenu();
}

void TrayHelper::callback_quit(void* userdata, SDL_TrayEntry* invoker)
{
    QWACK_LOG("Program terminated through tray icon.");
    SDL_Event e;
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}