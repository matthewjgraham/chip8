#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include "bitmanip.h"
#include "chip8.h"
#include "screen.h"

struct CHIP8 theCHIP8;

extern uint8 keysPressed[16];

static void*
startTimers(void*)
{
    struct timespec ts = { 
        .tv_sec = 0, 
        .tv_nsec = 16.666666 * 1000000 /* for 60 sleeps per second*/
    };
    for (;;) {
        if (theCHIP8.soundTimer > 0) {
            printf("I am happening!\n");
            printf("\a");
            --theCHIP8.soundTimer;
        }
        if (theCHIP8.delayTimer > 0)
            --theCHIP8.delayTimer;
        nanosleep(&ts, NULL);
    }
    return NULL;
}

void initializeCHIP8Graphics();

static pthread_t timerThread;

static void 
resetCHIP8()
{
    clearScreen();
    memset(theCHIP8.stack, 0, STACK_SIZE);
    memset(theCHIP8.ram, 0, RAM_SIZE);
    memset(theCHIP8.registers, 0, 16);
    theCHIP8.stackPtr = 0;
    theCHIP8.delayTimer = 0;
    theCHIP8.soundTimer = 0;
    theCHIP8.PC = INSTRUCTION_ADDRESS_BASE;
    theCHIP8.I = 0;
}
void
initializeCHIP8()
{
    resetCHIP8();
    srand(time(NULL));

    pthread_create(&timerThread, NULL, startTimers, NULL);

    void initializeCHIP8Graphics();
    initializeCHIP8Graphics();
}

static uint8 
carryAdd(uint8 x, uint8 y)
{
    uint8 result = x + y;
    if (255 - y < x) {
        theCHIP8.registers[0xF] = 1;
    } else {
        theCHIP8.registers[0xF] = 0;
    }
    return result;
}

static uint8 
borrowSub(uint8 x, uint8 y)
{
    uint8 result = x - y;
    if (x < y) {
        theCHIP8.registers[0xF] = 0;
    } else {
        theCHIP8.registers[0xF] = 1;
    }
    return result;
}

__attribute__((noreturn))
void
errorAndExit(char const* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    dumpCPUState();
    exit(1);
}


#define errorAndExitOnFail$(expression, msg) if (!(expression)) errorAndExit("Assertion failed: " #expression "\n" msg "\n")
#define errorAndExitOnFailf$(expression, msg, ...) if (!(expression)) errorAndExit("Assertion failed: " #expression "\n" msg "\n", __VA_ARGS__)


void
loadROMFromPath(char const* path)
{
    FILE* fp = fopen(path, "rb");
    errorAndExitOnFail$(fp, "chip8: Please enter a valid path!");
    int amountRead = fread(&theCHIP8.ram[INSTRUCTION_ADDRESS_BASE], 1, RAM_SIZE - INSTRUCTION_ADDRESS_BASE, fp);
    fclose(fp);
}


static void
stackPopPC()
{
    errorAndExitOnFail$(theCHIP8.stackPtr != 0, "Stack underflow");
    theCHIP8.PC = theCHIP8.stack[theCHIP8.stackPtr -= 2];
}

static void 
stackPushPC()
{
    errorAndExitOnFail$(theCHIP8.stackPtr != STACK_SIZE - 1, "Stack overflow!");
    theCHIP8.stack[theCHIP8.stackPtr] = theCHIP8.PC;
    theCHIP8.stackPtr += 2;
}


static Instruction
fetchNextInstruction()
{
    assert(theCHIP8.PC != RAM_SIZE);
    // converts to little endian, only correct on little endian systems like amd64
    Instruction inst = __builtin_bswap16(*(Instruction*)&theCHIP8.ram[theCHIP8.PC]);
    theCHIP8.PC += 2;
    return inst;
}

void
doNextInstruction()
{
    Instruction inst = fetchNextInstruction();
    //printf("Inst: %x\n", inst);
    switch (firstNibble$(inst)) {
    case 0:
        if (secondNibble$(inst) == 0) {
            if (thirdNibble$(inst) == 0xE) {
                byte fourth = fourthNibble$(inst);
                if (fourth == 0) { /* CLS */
                    clearScreen();
                    break;
                } else if (fourth == 0xE) { /* RET */
                    stackPopPC();
                    break;
                }
            }
        }
    //	theCHIP8.PC = decodeNNN$(inst); 
	break;
    case 1:
        theCHIP8.PC = decodeNNN$(inst);
	    break;
    case 2:
        stackPushPC();
        theCHIP8.PC = decodeNNN$(inst);
        break;
    case 3:
        if (theCHIP8.registers[decodeX$(inst)] == decodeNN$(inst))
            theCHIP8.PC += 2;
        break;
    case 4:
        if (theCHIP8.registers[decodeX$(inst)] != decodeNN$(inst))
            theCHIP8.PC += 2;
        break;
    case 5:
        if (theCHIP8.registers[decodeX$(inst)] == theCHIP8.registers[decodeY$(inst)])
            theCHIP8.PC += 2;
        break;
    case 6:
        theCHIP8.registers[decodeX$(inst)] = decodeNN$(inst);
        break;
    case 7:
        theCHIP8.registers[decodeX$(inst)] = carryAdd(theCHIP8.registers[decodeX$(inst)], decodeNN$(inst));
        break;
    case 8:
        switch (decodeN$(inst)) {
        case 0:
            theCHIP8.registers[decodeX$(inst)] = theCHIP8.registers[decodeY$(inst)];
            break;
        case 1:
            theCHIP8.registers[decodeX$(inst)] |= theCHIP8.registers[decodeY$(inst)];
            break;
        case 2:
            theCHIP8.registers[decodeX$(inst)] &= theCHIP8.registers[decodeY$(inst)];
            break;
        case 3:
            theCHIP8.registers[decodeX$(inst)] ^= theCHIP8.registers[decodeY$(inst)];
            break;
        case 4:
            theCHIP8.registers[decodeX$(inst)] = carryAdd(theCHIP8.registers[decodeX$(inst)], theCHIP8.registers[decodeY$(inst)]);
            break;
        case 5:
            theCHIP8.registers[decodeX$(inst)] = borrowSub(theCHIP8.registers[decodeX$(inst)], theCHIP8.registers[decodeY$(inst)]);
            break;
        case 6: /* uses the new way of shifting, in place */
            theCHIP8.registers[0xF] = theCHIP8.registers[decodeX$(inst)] & 0b1;
            theCHIP8.registers[decodeX$(inst)] >>= 1;
            break;
        case 7:
            theCHIP8.registers[decodeX$(inst)] = borrowSub(theCHIP8.registers[decodeY$(inst)], theCHIP8.registers[decodeY$(inst)]);
            break;
        case 0xE:
            theCHIP8.registers[0xF] = theCHIP8.registers[decodeX$(inst)] >> 7;
            theCHIP8.registers[decodeX$(inst)] <<= 1;
            break;
        }
        break;
    case 9:
        if (theCHIP8.registers[decodeX$(inst)] != theCHIP8.registers[decodeY$(inst)])
            theCHIP8.PC += 2;
        break;
    case 0xA:
        theCHIP8.I = decodeNNN$(inst);
        break;
    case 0xB:
        theCHIP8.I = decodeNNN$(inst) + theCHIP8.registers[0];
        break;
    case 0xC:
        theCHIP8.registers[decodeX$(inst)] = rand() & decodeNN$(inst);
        break;
    case 0xD:
        displaySprite(inst);
        break;
    case 0xE:
        if (decodeNN$(inst) == 0x9E) {
            if (keysPressed[theCHIP8.registers[decodeX$(inst)]]) {
                theCHIP8.PC += 2;
            }
            break;
        } else if (decodeNN$(inst) == 0xA1) {
            if (!keysPressed[theCHIP8.registers[decodeX$(inst)]]) {
                theCHIP8.PC += 2;
            }
            break;
        } else {
            errorAndExit("Bad instruction: %X\n", inst);
        }
    case 0xF:
        switch (thirdNibble$(inst)) {
        case 0:
            if (decodeN$(inst) == 7) {
                theCHIP8.registers[decodeX$(inst)] = theCHIP8.delayTimer;
                break;
            } else if (decodeN$(inst) == 0xA) {
                theCHIP8.registers[decodeX$(inst)] = (byte)waitForKeyPress();
                break;
            }
            errorAndExit("Bad instruction: %X\n", inst);
        case 1:
            switch (decodeN$(inst)) {
            case 5:
                theCHIP8.delayTimer = theCHIP8.registers[decodeX$(inst)];
                break;
            case 8:
                theCHIP8.soundTimer = theCHIP8.registers[decodeX$(inst)];
                break;
            case 0xE:
                theCHIP8.I += theCHIP8.registers[decodeX$(inst)];
                theCHIP8.registers[0xF] = (theCHIP8.I > 0xFFF) ? 1 : 0;
                break;
            default:
                errorAndExit("Bad instruction: %X\n", inst);
            }
            break;
        case 2:
            if (fourthNibble$(inst) != 9) {
                errorAndExit("Bad instruction: %X\n", inst);
            }
            theCHIP8.I = (theCHIP8.registers[decodeX$(inst)] & 0x0F) * 5;
            break;
        case 3:
            if (fourthNibble$(inst) != 3) {
                errorAndExit("Bad instruction: %X\n", inst);
            }
            uint8 num = theCHIP8.registers[decodeX$(inst)];
            theCHIP8.ram[theCHIP8.I] = num / 100;
            theCHIP8.ram[theCHIP8.I + 1] = (num / 10) % 10;
            theCHIP8.ram[theCHIP8.I + 2] = num % 10;
            break;
        case 5:                                
            memcpy(&theCHIP8.ram[theCHIP8.I], theCHIP8.registers, decodeX$(inst) + 1);
            break;
        case 6:
            memcpy(theCHIP8.registers, &theCHIP8.ram[theCHIP8.I], decodeX$(inst) + 1);
            break;
        default:
            errorAndExit("Bad instruction: %X\n", inst);
        }
        break;
    default:
        errorAndExit("Bad instruction: %X\n", inst);
    }
    struct timespec ts = { 
        .tv_sec = 0, 
        .tv_nsec = 1 * 1000000
    };
    nanosleep(&ts, &ts);
}

void
dumpCPUState()
{
    printf("-------- CPU STATE --------\n");
    printf("PC: %X\n", theCHIP8.PC);
    printf("I: %X\n", theCHIP8.I);
    printf("stackPtr: %X\n", theCHIP8.stackPtr);
    printf("Value at stackPtr: %X\n", theCHIP8.stack[theCHIP8.stackPtr]);
    printf("Registers:\n");
    for (uint8 i = 0; i < 16; i++) {
        printf("  V[%X] = %X\n", i, theCHIP8.registers[i]);
    }
}
