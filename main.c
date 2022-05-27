#include <assert.h>
#include "chip8.h"
#include "screen.h"

void initializeCHIP8();
void sdlEventLoop();

int main(int argc, char** argv)
{
    assert(argc == 2 && "usage: chip8 [rom]");

    initializeCHIP8();

    loadROMFromPath(argv[1]);

    sdlEventLoop();

}
