#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include "bitmanip.h"
#include "chip8.h"
#include "screen.h"

uint8 keysPressed[16] = { 0 };

uint8 fontCharacters[] = {
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


static Pixel internalBitmap[SCREEN_SIZE] = { 0 };

void
initializeCHIP8Graphics()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }
    Screen* screen = &theCHIP8.screen;
    screen->win = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 512, SDL_WINDOW_SHOWN);
    screen->rend = SDL_CreateRenderer(screen->win, -1, 0);
    SDL_RenderSetLogicalSize(screen->rend, 1024, 512);
    screen->tex = SDL_CreateTexture(screen->rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_RenderPresent(screen->rend);
    memcpy(theCHIP8.ram, fontCharacters, 5 * 16);
}

void
destroyCHIP8Screen()
{
    SDL_DestroyWindow(theCHIP8.screen.win);
    SDL_Quit();
}

void
clearScreen()
{
    memset(internalBitmap, 0, sizeof(internalBitmap));
    SDL_Texture* tex = theCHIP8.screen.tex;
    SDL_UpdateTexture(tex, NULL, internalBitmap, SCREEN_WIDTH * sizeof(Pixel));

    SDL_Renderer* rend = theCHIP8.screen.rend;
    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
}

void
clearKeys()
{
    memset(keysPressed, 0, sizeof(keysPressed));
}

static int chip8ToSDLKeys[16] = {
    [1] = SDL_SCANCODE_1,
    [2] = SDL_SCANCODE_2,
    [3] = SDL_SCANCODE_3,
    [0xC] = SDL_SCANCODE_4,
    [4] = SDL_SCANCODE_Q,
    [5] = SDL_SCANCODE_W,
    [6] = SDL_SCANCODE_E,
    [0xD] = SDL_SCANCODE_R,
    [7] = SDL_SCANCODE_A,
    [8] = SDL_SCANCODE_S,
    [9] = SDL_SCANCODE_D,
    [0xE] = SDL_SCANCODE_F,
    [0xA] = SDL_SCANCODE_Z,
    [0] = SDL_SCANCODE_X,
    [0xB] = SDL_SCANCODE_C,
    [0xF] = SDL_SCANCODE_V,
};


static uint8 sdlToCHIP8Keys[300] = {
    0,
    [SDL_SCANCODE_1] = 1,
    [SDL_SCANCODE_2] = 2,
    [SDL_SCANCODE_3] = 3,
    [SDL_SCANCODE_4] = 0xC,
    [SDL_SCANCODE_Q] = 4,
    [SDL_SCANCODE_W] = 5,
    [SDL_SCANCODE_E] = 6,
    [SDL_SCANCODE_R] = 0xD,
    [SDL_SCANCODE_A] = 7,
    [SDL_SCANCODE_S] = 8,
    [SDL_SCANCODE_D] = 9,
    [SDL_SCANCODE_F] = 0xE,
    [SDL_SCANCODE_Z] = 0xA,
    [SDL_SCANCODE_X] = 0,
    [SDL_SCANCODE_C] = 0xB,
    [SDL_SCANCODE_V] = 0xF,
};

void
setPressedKeys()
{
    uint8 const* keyState = SDL_GetKeyboardState(NULL);
    for (size_t i = 0; i < 16; i++) {
        if (keyState[chip8ToSDLKeys[i]]) {
            keysPressed[i] = 1;
        }
    }
}

void
displaySprite(Instruction inst)
{
    SDL_Renderer* rend = theCHIP8.screen.rend;
    SDL_Texture* tex = theCHIP8.screen.tex;
    byte xCoord = theCHIP8.registers[decodeX$(inst)] % SCREEN_WIDTH;
    byte yCoord = theCHIP8.registers[decodeY$(inst)] % SCREEN_HEIGHT;
    byte height = decodeN$(inst);

    theCHIP8.registers[0xF] = 0;

    for (uint16 i = 0; i < height && i + yCoord < SCREEN_HEIGHT; i++) {
        byte p = theCHIP8.ram[theCHIP8.I + i];
        for (uint8 j = 0; j < 8 && j + xCoord < SCREEN_WIDTH; j++) {
            if (p & (0x80 >> j)) {
                Pixel* bmpP = &internalBitmap[SCREEN_WIDTH * (yCoord + i) + (xCoord + j)];
                if (*bmpP) {
                    theCHIP8.registers[0xF] = 1;
                }
                *bmpP ^= 0xFFFFFFFF;
            }
        }
    }

    SDL_UpdateTexture(tex, NULL, internalBitmap, SCREEN_WIDTH * sizeof(Pixel));
    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
}


void
sdlEventLoop()
{
    for (;;) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                dumpCPUState();
                exit(0);
            case SDL_KEYDOWN:
                setPressedKeys();
                break;
            case SDL_KEYUP:
                clearKeys();
                break;
            }
        }
        doNextInstruction();
    }
}




char
waitForKeyPress()
{
    char whatKeyIsPressed;
begin:
    for (;;) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                dumpCPUState();
                exit(0);
            case SDL_KEYDOWN:
               return sdlToCHIP8Keys[(char)event.key.keysym.scancode];
            case SDL_KEYUP:
               continue;
            }
        }

        struct timespec ts = { 
            .tv_sec = 0, 
            .tv_nsec = 1 * 1000000
        };
        nanosleep(&ts, &ts);
    }
end: return whatKeyIsPressed;
}


