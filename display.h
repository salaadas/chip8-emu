#pragma once
#include <SDL.h>
#include <bitset>

class Display {
    public:
    Display();
    ~Display();
    void update();
    void update(const SDL_Rect &rect);
    void clear();
    uint8_t blit(uint8_t *src, uint8_t size, uint8_t x, uint8_t y);

    private:
    static const uint8_t WIDTH = 64;
    static const uint8_t HEIGHT = 64;
    static const uint8_t PIXEL_SIZE = 12;
    SDL_Rect screen_rect;

    SDL_Window *window;
    SDL_Renderer *renderer;
    std::bitset<WIDTH * HEIGHT> gfx;

    bool setPixel(int16_t x, int16_t y);
};
