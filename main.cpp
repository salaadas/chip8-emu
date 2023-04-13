#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <time.h>

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
uint8_t dt, st; // delay timer, sound timer
// TODO: Add keyboard poller

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

bool is_keydown(uint4_t key) {
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);
    if (key < 0 || key > 15) return false;
    real_key = keys[key];
    return keystate[real_key];
}

void init(void)
{
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;
    srand(time(NULL)); // set random seed
    
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
    if (wait_key != -1) {
        for (int i = 0; i < 16; ++i) {
            int status = is_keydown(i);
            if (status) {
                V[(uint4_t)wait_key] = i;
                wait_key = -1
            }
        }
        if (wait_key != 1) {
            return;
        }
    }

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
                    --sp;
                    pc = stack[sp];
                    pc += 2;
                } break;
            }
        } break;
        // 0x1NNN jump to address NNN
        case 0x1000: {
            pc = opcode & 0x0FFF;
        } break;
        // 0x2NNN call the subroutine at NNN
        case 0x2000: {
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
        } break;
        // 0x3XNN skip next instruction if Vx == NN
        case 0x3000: {
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
        } break;
        // 0x4XNN skip next instruction if Vx != NN
        case 0x4000: {
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 4;
            } else {
                pc += 2;
            }
        } break;
        // 0x5XY0 skip next instruction if Vx == Vy
        case 0x5000: {
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00F0)) {
                pc += 4;
            } else {
                pc += 2;
            }
        } break;
        // 0x6XNN set Vx to NN
        case 0x6000: {
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
        } break;
        // 0x7XNN add NN to Vx (carry flag unchanged)
        case 0x7000: {
            uint4_t x = (opcode & 0x0F00) >> 8;
            V[x] += (x == 15) ? 0 : (opcode & 0x00FF);
            pc += 2;
        } break;
        // 0x8XY_
        case 0x8000: {
            switch(opcode & 0x000F) {
                // 0x8XY0 set Vx = Vy
                case 0x0000: {
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                } break;
                // 0x8XY1 set Vx |= Vy (bitwise OR)
                case 0x0001: {
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];                    
                    pc += 2;
                } break;
                // 0x8XY2 set Vx &= Vy (bitwise AND)
                case 0x0002: {
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                } break;
                // 0x8XY3 set Vx ^= Vy (bitwise XOR)
                case 0x0003: {
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                } break;
                // 0x8XY4 set Vx += Vy
                case 0x0004: {
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    // check for setting carry Vf
                    if (V[(opcode & 0x0F00) >> 8] > (0xFF - V[(opcode & 0x00F0) >> 4])) {
                        V[0xF] = 1; // if there is a carry
                    } else {
                        V[0xF] = 0;
                    }
                    pc += 2;
                } break;
                // 0x8XY5 set Vx -= Vy
                case 0x0005: {
                    if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
                        V[0xF] = 0; // set carry flag to 0 when there is a borrow
                    } else {
                        V[0xF] = 1; // set flag to 1 when there is not
                    }
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                } break;
                // 0x8XY6 Stores the least significant bit of VX in VF and then shifts VX to the right by 1
                case 0x0006: {
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                } break;
                // 0x8XY7 Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not
                case 0x0007: {
                    uint4_t x = (opcode & 0x0F00) >> 8;
                    uint4_t y = (opcode & 0x00F0) >> 4;
                    if (V[x] > V[y]) {
                        V[0xF] = 0; // there's a borrow
                    } else {
                        V[0xF] = 1;
                    }
                    V[x] = V[y] - V[x];
                    pc += 2;
                } break;
                // 0x8XYE Stores the least significant bit of VX in VF and then shifts VX to the right by 1
                case 0x000E: {
                    V[0xF] = V[(opcode & 0x0F00) >> 8] >> 15;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                } break;

                default: {
                    fprintf(stderr, "ERROR: Unreachable, no opcodes of such %.4X\n", opcode);
                    exit(3);
                }
            }
        } break;
        // 0x9XY0 Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
        case 0x9000: {
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
                pc += 4;
            } else {
                pc += 2;
            }
        } break;
        // 0xANNN set I to address NNN
        case 0xA000: {
            I = opcode & 0x0FFF;
            pc += 2;
        } break;
        // 0xBNNN Jumps to the address NNN plus V0.
        case 0xB000: {
            pc = opcode & 0x0FFF + V[0];
            pc += 2;
        } break;
        // 0xCXNN Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
        case 0xC000: {
            V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
            pc += 2;
        } break;
        // 0xDXYN
        // Draws a sprite at coordinate [(VX, VY)] that has a width of [8] pixels and a height of [N] pixels.
        // [Each row] of 8 pixels is read as bit-coded starting from [memory] location [I];
        // [I] value [does not change] after the execution of this instruction.
        // As described above, [VF] is set to 1 if any screen pixels are flipped from set to unset
        // when the sprite is drawn, and to 0 if that does not happen.
        // Draw using the XOR operator 
        case 0xD000: {
            uint4_t n = opcode & 0x000F;
            uint4_t vx = (opcode & 0x0F00) >> 8;
            uint4_t vy = (opcode & 0x00F0) >> 4;

            V[0xF] = 0;

            for (int dy = 0; dy < n; ++dy) {
                uint8_t pixel = memory[I + dy];
                for (int dx = 0; dx < 8; ++dx) {
                    // scan through the pixels, one bit at a time
                    if (pixel & (0x80 >> dx)) {
                        size_t index = ((vy + dy) * WIDTH * SCALE) + (vx + dx) * SCALE;
                        if (gfx[index] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[index] ^= 1;
                    }
                }
            }
            DrawFlag = true;
            pc += 2;
        } break;
        // 0xEX__
        // Handling input
        case 0xE000: {
            // TODO: Add polling input for 0xEX__
            switch (opcode & 0x00FF) {
                // Skips the next instruction if the key stored in VX
                // is PRESSED (usually the next instruction is a jump to skip a code block).
                case 0x009E: {
                    if (is_keydown(V[(opcode & 0x0F00) >> 8])) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                } break;
                // Skips the next instruction if the key stored in VX
                // is NOT PRESSED (usually the next instruction is a jump to skip a code block).
                case 0x00A1: {
                    if (!is_keydown(V[(opcode & 0x0F00) >> 8])) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                } break;
                default: {
                    fprintf(stderr, "ERROR: Unreachable, no opcodes of such %.4X\n", opcode);
                    exit(3);
                }
            }
        } break;
        // 0xFX__
        case 0xF000: {
            switch (opcode & 0x00FF) {
                // VX = dt
                case 0x0007: {
                    V[opcode & (0x0F00) >> 8] = dt;
                } break;
                case 0x000A: {
                    wait_key = V[opcode & (0x0F00) >> 8];
                } break;
                case 0x0007: {
                    
                } break;
                case 0x0007: {
                    
                } break;
                case 0x0007: {
                    
                } break;
                case 0x0007: {
                    
                } break;
                case 0x0007: {
                    
                } break;
                case 0x0007: {
                    
                } break;
                case 0x0007: {
                    
                } break;
            }
        } break;
        default: {
            fprintf(stderr, "ERROR: Opcode unimplemented %.4X\n", opcode);
            exit(1);
        }
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
