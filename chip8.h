#pragma once

#include "defs.h"
#include "screen.h"

extern struct CHIP8 {
    Screen screen;
    uint16 PC;
    _Atomic uint8 delayTimer;
    _Atomic uint8 soundTimer;
    uint16 I;
    Register registers[16];
    uint16 stack[STACK_SIZE];
    uint16 stackPtr;
    byte ram[RAM_SIZE];
} theCHIP8;

#define INSTRUCTION_ADDRESS_BASE 0x200

void loadROMFromPath(char const*);
void doNextInstruction();
void dumpCPUState();
