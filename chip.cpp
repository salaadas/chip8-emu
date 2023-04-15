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

void Chip::pressKey(const uint8_t key)
{
    keys[key] = true;

    if (is_waiting) {
        V[wait_key_reg] = key;
        is_waiting = false;
    }
}

void Chip::releaseKey(const uint8_t key)
{
    keys[key] = false;
}
 
bool Chip::waitKey(void)
{
    return is_waiting;
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
        std::cout << std::hex << "V" << i << ": " << std::setw(4) << (unsigned)V[i] << std::endl;
    }
    std::cout << std::endl;
}

void Chip::step(void)
{
    fetchCode(); // get next opcode
    const uint8_t MSB = ir >> 12; // most significant bit
    const uint8_t LSB = ir & 0x000F; // least significant bit
    const uint8_t X = (ir & 0x0F00) >> 8;
    const uint8_t Y = (ir & 0x00F0) >> 4;
    const uint8_t KK = ir & 0x00FF;
    const uint16_t NNN = ir & 0x0FFF;
    const uint8_t VX = V[X];
    const uint8_t VY = V[Y];

    // for 
    auto D_X = (unsigned)X;
    auto D_Y = (unsigned)Y;
    auto D_KK = (unsigned)KK;
    auto D_NNN = (unsigned)NNN;
    std::cout << std::hex << std::setw(4) << (unsigned)pc << ' ' << std::setw(4) << (unsigned)ir << ' ' << std::setw(2) << (unsigned)sp << ' ';

    switch(MSB) {
        case 0x0: {
            switch(KK) {
                case 0xE0: {
                    std::cout << std::hex << "CLS\n";
                    display->clear();
                    pc += 2;
                } break;
                case 0xEE: {
                    std::cout << std::hex << "RET\n";
                    pc = stack[--sp];
                    break;
                } break;
                default: {
                    failure(ir);
                } break;
            }
        } break;
        case 0x1: {
            std::cout << std::hex << "JP " << D_NNN << std::endl;
            pc = NNN;
        } break;
        case 0x2: {
            std::cout << std::hex << "CALL " << D_NNN << std::endl;
            stack[sp++] = pc + 2;
            pc = NNN;
        } break;
        case 0x3: {
            std::cout << std::hex << "SE V" << D_X << ", " << D_KK << std::endl;
            pc += (VX == KK) ? 4 : 2;
        } break;
        case 0x4: {
            std::cout << std::hex << "SNE V" << D_X << ", " << D_KK << std::endl;
            pc += (VX != KK) ? 4 : 2;
        } break;
        case 0x5: {
            std::cout << std::hex << "SE V" << D_X << ", V" << D_Y << std::endl;
            pc += (VX == VY) ? 4 : 2;
        } break;
        case 0x6: {
            std::cout << std::hex << "LD V" << D_X << ", " << D_KK << std::endl;
            V[X] = KK;
            pc += 2;
        } break;
        case 0x7: {
            std::cout << std::hex << "ADD V" << D_X << ", " << D_KK << std::endl;
            V[X] += (X == 15) ? 0 : KK;
            pc += 2;
        } break;
        case 0x8: {
            switch(LSB) {
                case 0x0: {
                    std::cout << std::hex << "LD V" << D_X << ", V" << D_Y << std::endl;
                    V[X] = VY;
                } break;
                case 0x1: {
                    std::cout << std::hex << "OR V" << D_X << ", V" << D_Y << std::endl;
                    V[X] |= VY;
                } break;
                case 0x2: {
                    std::cout << std::hex << "AND V" << D_X << ", V" << D_Y << std::endl;
                    V[X] &= VY;
                } break;
                case 0x3: {
                    std::cout << std::hex << "XOR V" << D_X << ", V" << D_Y << std::endl;
                    V[X] ^= VY;
                } break;
                case 0x4: {
                    std::cout << std::hex << "ADD V" << D_X << ", V" << D_Y << std::endl;
                    V[X] += VY;
                    auto x16 = static_cast<uint16_t>(VX);
                    auto y16 = static_cast<uint16_t>(VY);
                    if (x16 > 0xFF - y16) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                } break;
                case 0x5: {
                    std::cout << std::hex << "SUB V" << D_X << ", V" << D_Y << std::endl;
                    V[0xF] = VX > VY ? 1 : 0;
                    V[X] -= VY;
                } break;
                case 0x6: {
                    std::cout << std::hex << "SHR V" << D_X << ", {V" << D_Y << "}" << std::endl;
                    V[0xF] = (LSB == 0x1) ? 1 : 0;
                    V[X] >>= 1;
                } break;
                case 0x7: {
                    std::cout << std::hex << "SUBN V" << D_X << ", V" << D_Y << std::endl;
                    V[0xF] = VY > VX ? 1 : 0;
                    V[X] = VY - VX;
                } break;
                case 0xE: {
                    std::cout << std::hex << "SHL V" << D_X << ", {V" << D_Y << "}" << std::endl;
                    V[0xF] = (MSB == 0x1) ? 1 : 0;
                    V[X] <<= 1;
                } break;
                default: {
                    failure(ir);
                }
            }
            pc += 2;
        } break;
        case 0x9: {
            std::cout << std::hex << "SNE V" << D_X << ", V" << D_Y << std::endl;
            pc += (VX != VY) ? 4 : 2;
        } break;
        case 0xA: {
            std::cout << std::hex << "LD I" << D_NNN << std::endl;
            I = NNN;
            pc += 2;
        } break;
        case 0xB: {
            std::cout << std::hex << "LD I" << D_NNN + V[0] << std::endl;
            I = NNN + V[0];
            pc += 2;
        } break;
        case 0xC: {
            std::cout << std::hex << "RND V" << D_X << ", " << D_KK << '\n';
            V[X] = (rnd() % 256) & (KK);
            pc += 2;
        } break;
        case 0xD: {
            V[0xF] = display->blit(&memory[I], LSB, VX, VY);
            pc += 2;
        } break;
        case 0xE: {
            switch(KK) {
                case 0x9E: {
                    std::cout << std::hex << "SKP V" << D_X << std::endl;
                    pc += keys[VX] ? 4 : 2;
                } break;
                case 0xA1: {
                    std::cout << std::hex << "SKNP V" << D_X << std::endl;                    
                    pc += !keys[VX] ? 4 : 2;
                } break;
                default: {
                    failure(ir);
                } break;
            }
        } break;
        case 0xF: {
            switch(KK) {
                case 0x0A: {
                    std::cout << std::hex << "LD V" << D_X << ", K" << std::endl;
                    is_waiting = true;
                    wait_key_reg = VX;
                } break;
                case 0x07: {
                    std::cout << std::hex << "LD V" << D_X << ", DT" << std::endl;
                    V[X] = dt;
                } break;
                case 0x15: {
                    std::cout << std::hex << "LD DT, V" << D_X << std::endl;
                    dt = VX;
                } break;
                case 0x18: {
                    std::cout << std::hex << "LD ST, V" << D_X << std::endl;
                    st = VX;
                } break;
                case 0x29: {
                    std::cout << std::hex << "LD F, V" << D_X << std::endl;
                    I = VX * 5;
                } break;
                case 0x33: {
                    std::cout << std::hex << "LD B, V" << D_X << std::endl;
                    memory[I]     = VX % 1000 / 100;
                    memory[I + 1] = VX % 100 / 10;
                    memory[I + 2] = VX % 10 / 1l;
                } break;
                case 0x55: {
                    std::cout << std::hex << "LD [i], V" << D_X << std::endl;
                    for (auto i = 0; i <= X; ++i) {
                        memory[I + i] = V[i];
                    }
                } break;
                case 0x1E: {
                    std::cout << std::hex << "ADD I, V" << D_X << std::endl;
                    I += VX;
                } break;
                case 0x65: {
                    std::cout << std::hex << "LD V" << D_X << ", [I]" << std::endl;
                    for (auto i = 0; i <= X; ++i) {
                        V[i] = memory[I + i];
                    }
                } break;
                default: {
                    failure(ir);
                } break;
            }
            pc += 2;
        } break;
        default: {
            failure(ir);
        } break;
    }
}

void Chip::failure(uint16_t ins)
{
    std::ostringstream oss;
    oss << std::hex << "Unknown Instruction: " << ins;
    throw std::runtime_error(oss.str());
}

void Chip::tick(void)
{
    if(dt > 0) dt -= 1;
    if(st > 0) st -= 1;
}

void Chip::fetchCode(void)
{
    ir = getDWord(pc);
}

uint16_t Chip::getDWord(uint16_t addr)
{
    return (static_cast<uint16_t>(memory[addr]) << 8) | memory[addr + 1];
}
