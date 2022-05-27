#pragma once

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef uint8 byte;
typedef uint8 Register;
typedef uint16 Instruction;

#define STACK_SIZE 4096
#define RAM_SIZE 4096

#define SCREEN_WIDTH (64)
#define SCREEN_HEIGHT (32)
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_WIDTH)
typedef uint32 Pixel;

typedef enum {
    false,
    true,
} bool;
