gcc -o sdl-01 sdl-01.c $(pkg-config --cflags --libs sdl3-ttf) -lSDL3_image -lcurl -ljson-c -lm

gcc -o sdl-10 sdl-10.c \
    -I/usr/include/SDL3_ttf -I/usr/include/SDL3 \
    -lSDL3_ttf -lSDL3_image -lSDL3 \
    -lfreetype -lharfbuzz -lm \
    -lcurl -ljson-c
