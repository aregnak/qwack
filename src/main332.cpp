#include <SDL.h>
#include <stdio.h>

int main(int argc, char** argv) {
    // Initialize SDL2 video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Window dimensions
    int width = 300;
    int height = 200;

    // Create a borderless, always-on-top window
    SDL_Window* window = SDL_CreateWindow(
        "SDL2 Test Window",                   // title
        SDL_WINDOWPOS_CENTERED,               // x
        SDL_WINDOWPOS_CENTERED,               // y
        width,                                // w
        height,                               // h
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP
    );

    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Show the window
    SDL_ShowWindow(window);

    printf("Window created! Press any key to exit...\n");
    getchar(); // wait for user input

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
