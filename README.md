gcc -o sdl-01 sdl-01.c $(pkg-config --cflags --libs sdl3-ttf) -lSDL3_image -lcurl -ljson-c -lm
