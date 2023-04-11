#include <stdio.h>
#include <SDL.h>

typedef enum {
    QUIT,
    RUN
} State;

constexpr int WIDTH = 64;
constexpr int HEIGHT = 32;
constexpr int SCALE = 10;
constexpr unsigned char chip8_font[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// CPU
uint8_t V[16];
uint16_t I;
uint16_t Pc;
uint16_t Stack[16];
uint16_t Sp; // stack pointer
uint16_t delay;

// Ram

int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "ERROR: Could not init window: %s\n", SDL_GetError());
    }
    window = SDL_CreateWindow("Chip 8 Emu", 0, 0, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    State state = RUN;
    SDL_Event event;
    while (state) {
        SDL_PollEvent(&event);
        switch(event.type) {
        case SDL_QUIT: {
            state = QUIT;
        } break;
        }

        SDL_SetRenderDrawColor(
            renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
