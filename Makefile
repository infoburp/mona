CC = gcc
LIBS += $(shell pkg-config --libs cairo)
CFLAGS += -Wall -std=c99 -pedantic -O2 -ggdb -I. $(shell pkg-config --cflags cairo)

#WITH_SDL ?= yes
ifeq "$(WITH_SDL)" "yes"
    CFLAGS += $(shell sdl2-config --cflags) -DWITH_SDL
    LIBS += $(shell sdl2-config --libs)
endif

ifeq "$(WITH_CUDA)" "yes"
    LIBS += -lm -L/usr/local/cuda/lib64 -lcuda -lcudart
endif

all: mona

diff.o: Makefile diff.cu diff.c
ifeq "$(WITH_CUDA)" "yes"
	nvcc $(CFLAGS) -arch=sm_20 -c diff.cu
else
	$(CC) $(CFLAGS) -c diff.c
endif

mona: Makefile diff.o mona.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) diff.o mona.c -o mona

clean:
	rm -f mona *.o
