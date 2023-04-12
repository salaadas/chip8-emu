#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <SDL.h>

typedef enum {
    QUIT,
    RUN
} State;

constexpr int WIDTH = 64;
constexpr int HEIGHT = 32;
constexpr int SCALE = 10;
constexpr unsigned char fontset[80] = {
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

uint16_t opcode; // current opcode from the 35 opcodes
uint8_t memory[4096]; // 4kb memory storing binary data of ROM
uint8_t V[16]; // 15 8bit registers, the 16th carries flag
uint16_t I; // index register
uint16_t pc; // program counter value from (0x000) -> (0xFFF)

/*
Memory map:
0x000 - 0x1FF -> chip 8 interpreter
0x200 - 0xFFF -> ROM and RAM
0x050 - 0x0A0 -> built in fontset
0xF00 - 0xFFF -> display refresh
0xEA0 - 0xEFF -> call stack
*/

constexpr int screen_sz = WIDTH * HEIGHT * SCALE;
uint8_t gfx[screen_sz];
bool DrawFlag = false;

uint8_t keys[16] = {
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_4,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_r,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_f,
    SDLK_z,
    SDLK_x,
    SDLK_c,
    SDLK_v,
};

uint16_t stack[16];
uint16_t sp;

void init(void)
{
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    // clear the "screen" -- this is the screen of the VM
    for (int i = 0; i < screen_sz; ++i) {
        gfx[i] = 0;
    }

    // clear memory
    for (int i = 0; i < 4096; ++i) {
        memory[i] = 0;
    }
    
    // load font to memory header chunk
    for (int i = 0; i < 80; ++i) {
        memory[i] = fontset[i];
    }
}

bool load(const char *fp)
{
    // init();
    printf("Loading ROM: %s\n", fp);

    FILE *rom = fopen(fp, "rb");

    if (rom == NULL) {
        fprintf(stderr, "ERROR: Failed to load ROM\n");
        return false;
    }

    fseek(rom, 0, SEEK_END);
    long int rom_sz = ftell(rom);
    rewind(rom);

    char *rom_buffer = (char *)malloc(sizeof(char) * rom_sz);
    if (rom_buffer == NULL) {
        fprintf(stderr, "ERROR: Failed to alloc mem for ROM\n");
        return false;
    }

    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_sz, rom);
    if (result != rom_sz) {
        fprintf(stderr, "ERROR: Could not read ROM successfully\n");
        return false;
    }

    if (rom_sz < (4096 - 512)) {
        for (int i = 0; i < rom_sz; ++i) {
            memory[i + 512] = (uint8_t)rom_buffer[i]; // load rom into mem, starting from 0x200
        }
    } else {
        fprintf(stderr, "ERROR: ROM too large\n");
        return false;
    }

    fclose(rom);
    free(rom_buffer);

    return true;
}

void emulate(void) // emulate one cycle
{
    // opcode = memory[pc] << 8 | memory[pc + 1];
    opcode = 0xAC74;

    switch (opcode & 0xF000) {
        case 0x0000: {
            switch(opcode & 0x00FF) {
                // clear screen
                case 0x00E0: {
                    for (int i = 0; i < screen_sz; ++i) {
                        gfx[i] = 0;
                    }
                    DrawFlag = true;
                    pc += 2;
                } break;
                // return from subroutine
                case 0x00EE: {
                    sp--;
                    pc = stack[sp];
                    pc += 2;
                } break;
            }
        } break;
        case 0x1000: {
            pc = opcode & 0x0FFF;
        } break;
        case 0xA000: {
            I = opcode & 0x0FFF;
            pc += 2;
        } break;
    }
}

int main(int argc, char** args) 
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "ERROR: Could not init window: %s\n", SDL_GetError());
    }
    window = SDL_CreateWindow("Chip 8 Emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * SCALE, HEIGHT * SCALE, SDL_WINDOW_SHOWN);

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
