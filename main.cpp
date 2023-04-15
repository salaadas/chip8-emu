#include <stdio.h>
#include "display.h"
#include "chip.h"
#include <map>

int main(int argc, char **argv)
{
    constexpr int SPEED = 1000; // 1Hz

    if (argc < 2) {
        printf("[ERROR]: Need input file, lol\n");
        return -1;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[ERROR]: SDL Failed to init: %s\n", SDL_GetError());
        return -1;
    }

    Display display;
    display.clear();

    Chip chip(display);
    chip.load((std::string)argv[1]);

    // keyboard
    // 1234 -> 123C
    // qwer -> 456D
    // asdf -> 789E
    // zxcv -> A0BF
    std::map<SDL_Keycode, uint8_t> keymap;    
    keymap.insert({SDLK_1, 0x1});
    keymap.insert({SDLK_2, 0x2});
    keymap.insert({SDLK_3, 0x3});
    keymap.insert({SDLK_4, 0xC});
    keymap.insert({SDLK_q, 0x4});
    keymap.insert({SDLK_w, 0x5});
    keymap.insert({SDLK_e, 0x6});
    keymap.insert({SDLK_r, 0xD});
    keymap.insert({SDLK_a, 0x7});
    keymap.insert({SDLK_s, 0x8});
    keymap.insert({SDLK_d, 0x9});
    keymap.insert({SDLK_f, 0xE});
    keymap.insert({SDLK_z, 0xA});
    keymap.insert({SDLK_x, 0x0});
    keymap.insert({SDLK_c, 0xB});
    keymap.insert({SDLK_v, 0xF});
    
    SDL_Event e;
    bool running = true;
    auto last_tick = SDL_GetTicks();

    while (running) {
        auto start_tick = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN: {
                    const auto key = e.key.keysym.sym;
                    if (key == SDLK_ESCAPE) running = false;
                    if (keymap.find(key) != keymap.end()) {
                        // handling keypress
                        chip.pressKey(keymap[key]);
                    }
                } break;
                case SDL_KEYUP: {
                    const auto key = e.key.keysym.sym;
                    if (keymap.find(key) != keymap.end()) {
                        // handling key release
                        chip.releaseKey(keymap[key]);
                    }
                } break;
            }
        }

        // handling key wait
        if (!chip.waitKey()) {
            if(SDL_GetTicks() - last_tick > 16) {
                chip.tick();
                last_tick = SDL_GetTicks();
            }
            chip.step();
        }

        auto end_tick = SDL_GetTicks();
        if (end_tick - start_tick < 1000 / SPEED) {
            SDL_Delay(1000 / SPEED - (end_tick - start_tick));
        }
    }

    SDL_Quit();
    return 0;
}
