#include "trayHelper.h"

void callback_quit(void* userdata, SDL_TrayEntry* invoker)
{
    SDL_Event e;
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

TrayHelper::TrayHelper()
{
    tray = SDL_CreateTray(NULL, "My tray");

    // Create a context menu for the tray.
    tmenu = SDL_CreateTrayMenu(tray);

    // Create a button in the context menu.
    entry = SDL_InsertTrayEntryAt(tmenu, -1, "Quit", SDL_TRAYENTRY_BUTTON);

    // Set the callback for the button
    SDL_SetTrayEntryCallback(entry, callback_quit, NULL);
}

TrayHelper::~TrayHelper()
{
    SDL_DestroyTray(tray);
    //
}
