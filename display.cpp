#include "display.h"
#include <stdio.h>

// HEX_COLOR(0xRRGGBBAA)
#define HEX_COLOR(x)    \
(x >> (6 * 4) & 0xff), \
(x >> (4 * 4) & 0xff), \
(x >> (2 * 4) & 0xff), \
(x >> (0 * 4) & 0xff)

Display::Display(void)
{
    printf("%.2X %.2X %.2X %.2X\n", HEX_COLOR(0x33445566));
    window = SDL_CreateWindow("Chip 8 Emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    screen_rect.x = screen_rect.y = 0;
    screen_rect.w = WIDTH;
    screen_rect.h = HEIGHT;
}

Display::~Display(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void Display::update(void)
{
    SDL_SetRenderDrawColor(renderer, HEX_COLOR(0xff0000ff)); // You shoudn't see this red color
    SDL_RenderClear(renderer);
    update(screen_rect);
}

void Display::update(const SDL_Rect &rect)
{
    SDL_SetRenderDrawColor(renderer, HEX_COLOR(0x181818ff));
    SDL_Rect clear = {
        rect.x * PIXEL_SIZE, rect.y * PIXEL_SIZE, rect.w * PIXEL_SIZE, rect.h * PIXEL_SIZE
    };
    SDL_RenderFillRect(renderer, &clear);

    SDL_SetRenderDrawColor(renderer, HEX_COLOR(0xaa33aaff));
    for (size_t i = 0; i < rect.w * rect.h; ++i) {
        uint16_t pos_x = rect.x + i % rect.w;
        uint16_t pos_y = (rect.y + i / rect.w) * WIDTH;

        uint16_t pos = pos_x + pos_y;
        if (gfx[pos]) {
            SDL_Rect r = {
                rect.x * PIXEL_SIZE + (i % rect.w) * PIXEL_SIZE,
                rect.y * PIXEL_SIZE + i / rect.w * PIXEL_SIZE,
                PIXEL_SIZE, PIXEL_SIZE
            };
            SDL_RenderFillRect(renderer, &r);
        }
    }
    SDL_RenderPresent(renderer);
}

void Display::clear(void)
{
    for (size_t i = 0; i < WIDTH * HEIGHT; ++i) {
        gfx[i] = 0;
    }
    update();
}

uint8_t Display::blit(uint8_t *src, uint8_t size, uint8_t x, uint8_t y)
{
    bool pixel_cleared = false;
    // number of bytes to draw
    for (size_t h = 0; h < size; ++h) {
        auto pixel = src[h];
        for (auto bit = 0; bit < 8; ++bit) {
            // compare pixels against 0x10000000
            // to draw each bit one by one in the [pixel]
            if (pixel & 0b10000000) {
                pixel_cleared = setPixel(x + bit, y + h) || pixel_cleared;
            }
            pixel = pixel << 1; // iterate to the next bit
        }
    }

    update();
    return pixel_cleared ? 1 : 0;
}

bool Display::setPixel(int16_t x, int16_t y)
{
    while (x > WIDTH - 1) x -= WIDTH;
    while (x < 0) x += WIDTH;

    while (y > HEIGHT - 1) y -= HEIGHT;
    while (y < 0) y += HEIGHT;

    size_t pos = y * WIDTH + x;
    gfx.flip(pos);
    return !gfx[pos]; // return true if pixel was turned from ON to OFF
}
