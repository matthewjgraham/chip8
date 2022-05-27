CC := gcc
CFLAGS := -std=c2x -O2
OBJS := main.o chip8.o screen.o
LIBS := `sdl2-config --cflags --libs`
all: chip8

chip8: $(OBJS)
	$(CC) -o chip8 $(OBJS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean run

clean:
	rm -f chip8 $(OBJS)

run: chip8
	./chip8
