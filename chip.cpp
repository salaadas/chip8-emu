#include "chip.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

Chip::Chip(Display &d):
    memory(MEMORY_SZ),
    V(REG_COUNT),
    stack(STACK_SZ),
    keys(16)
{
    display = &d;
}

void Chip::reset() {
    pc = 0x200;
    ir = getDWord(pc);
    std::fill(memory.begin(), memory.end(), 0x0);
    fill(V.begin(), V.end(), 0x0);
    fill(stack.begin(), stack.end(), 0x0);
    fill(keys.begin(), keys.end(), false);
    sp = 0;
    I = 0x0;
    
    std::vector<uint8_t> fonts = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,
        0x20, 0x60, 0x20, 0x20, 0x70,
        0xF0, 0x10, 0xF0, 0x80, 0xF0,
        0xF0, 0x10, 0xF0, 0x10, 0xF0,
        0x90, 0x90, 0xF0, 0x10, 0x10,
        0xF0, 0x80, 0xF0, 0x10, 0xF0,
        0xF0, 0x80, 0xF0, 0x90, 0xF0,
        0xF0, 0x10, 0x20, 0x40, 0x40,
        0xF0, 0x90, 0xF0, 0x90, 0xF0,
        0xF0, 0x90, 0xF0, 0x10, 0xF0,
        0xF0, 0x90, 0xF0, 0x90, 0x90,
        0xE0, 0x90, 0xE0, 0x90, 0xE0,
        0xF0, 0x80, 0x80, 0x80, 0xF0,
        0xE0, 0x90, 0x90, 0x90, 0xE0,
        0xF0, 0x80, 0xF0, 0x80, 0xF0,
        0xF0, 0x80, 0xF0, 0x80, 0x80
    };

    copy(fonts.begin(), fonts.end(), memory.begin());
    is_waiting = false;
    wait_key_reg = 0;
    st = 0;
    dt = 0;
}

void Chip::load(const std::string &filename)
{
    reset();
    std::ifstream input(filename, std::ios::in | std::ios::binary);
    std::vector<char> program((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();
    copy(program.begin(), program.end(), memory.begin() + pc);
}

void Chip::dumpRegs(void)
{
    std::cout << std::hex << "pc: " << pc << ' ';
    std::cout << std::hex << "opcode: " << ir << ' ';
    std::cout << std::hex << "I: " << I << std::endl;
    for (auto i = 0; i < REG_COUNT; ++i) {
        std::cout << std::hex << "V" << i << ": " << std::setw(4) << unsigned(V[i]) << std::endl;
    }
    std::cout << std::endl;
}

void Chip::step(void)
{
    
}

void Chip::tick(void)
{
    if(dt > 0) dt -= 1;
    if(st > 0) st -= 1;
}

void Chip::fetchCode()
{
    ir = getDWord(pc);
}

uint16_t Chip::getDWord(uint16_t addr)
{
    return (static_cast<uint16_t>(memory[addr]) << 8) | memory[addr + 1];
}
