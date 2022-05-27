#pragma once

#include "defs.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

typedef struct {
    SDL_Window* win;
    SDL_Renderer* rend;
    SDL_Texture* tex;
} Screen;

extern bool isRedrawNeeded;

void clearScreen();
void displaySprite(Instruction x);
char waitForKeyPress();
