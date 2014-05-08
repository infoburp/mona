mona: Makefile mona.c
	gcc -DWITH_SDL -Wall -std=c11 -pedantic -O2 -ggdb `sdl2-config --libs --cflags` `pkg-config --libs --cflags cairo` mona.c -o mona
clean:
	rm -f mona

