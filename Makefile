mona: Makefile mona.c
	g++-mp-4.4 -DSHOWWINDOW -Wall -ffast-math -fno-math-errno -pedantic -O3 `pkg-config --libs --cflags cairo x11 cairo-xlib` mona.c -o mona
clean:
	rm -f mona

