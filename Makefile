mona: Makefile mona.c
	gcc -DSHOWWINDOW -Wall -std=gnu99 -pedantic -O3 `sdl2-config --libs --cflags` `pkg-config --libs --cflags cairo cairo-xlib` mona.c -o mona
clean:
	rm -f mona

