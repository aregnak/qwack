#include "trayHelper.h"

#include "log.h"
#include "menuWindow.h"

TrayHelper::TrayHelper(MenuWindow& menu)
    : _menu(menu)
{
    tray = SDL_CreateTray(NULL, "Qwack");

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