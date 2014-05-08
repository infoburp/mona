CC = gcc
LIBS += $(shell pkg-config --libs cairo)
CFLAGS += $(shell pkg-config --cflags cairo)

WITH_SDL ?= yes

ifeq "$(WITH_SDL)" "yes"
    CFLAGS += $(shell sdl2-config --cflags) -DWITH_SDL
    LIBS += $(shell sdl2-config --libs)
endif

mona: Makefile mona.c
	$(CC) -Wall -std=gnu99 -pedantic -O2 -ggdb $(CFLAGS) $(LDFLAGS) $(LIBS) mona.c -o mona
clean:
	rm -f mona
