mona: Makefile mona.c
	gcc -DSHOWWINDOW -Wall -O2 `pkg-config --libs --cflags cairo x11 cairo-xlib` -lpthread mona.c -o mona
clean:
	rm -f mona

