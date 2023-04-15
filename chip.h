#pragma once
#include <random>
#include "display.h"
#include <vector>
#include <string>

class Chip {
    public:
    Chip(Display &display);
    void reset();
    void load(const std::string &filename);
    void step();
    void tick();
    void dumpMemory();
    void dumpRegs();
    bool waitKey();
    void pressKey(const uint8_t);
    void releaseKey(const uint8_t);

    private:
    static const uint16_t MEMORY_SZ = 4096;
    static const uint16_t REG_COUNT = 16;
    static const uint16_t STACK_SZ = 16;

    std::vector<uint8_t> memory;
    uint16_t pc;
    uint16_t ir;
    std::vector<uint8_t> V;
    uint16_t I;
    std::vector<uint16_t> stack;
    uint8_t sp;
    uint8_t st;
    uint8_t dt;

    Display *display;
    std::random_device rnd;

    bool is_waiting;
    uint8_t wait_key_reg;
    std::vector<bool> keys;

    void fetchCode();
    uint16_t getDWord(uint16_t);
    // void decode_failure(uint16_t);
};
