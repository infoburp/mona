all: mona

diff.o: Makefile diff.cu diff.c
ifdef CUDA
	nvcc -c -arch=sm_20 diff.cu -L/usr/lib64 `pkg-config --libs --cflags cairo` -lm
else
	gcc -DWITH_SDL -Wall -std=c99 -pedantic -O2 -ggdb -I. \
	`sdl2-config --libs --cflags` \
	`pkg-config --libs --cflags cairo` \
	-c diff.c
endif

mona: Makefile diff.o mona.c
	gcc -DWITH_SDL -Wall -std=c99 -pedantic -O2 -ggdb -I. \
	-L/usr/local/cuda/lib64 -lcuda -lcudart \
	`sdl2-config --libs --cflags` \
	`pkg-config --libs --cflags cairo` -lm \
	diff.o mona.c -o mona
	
clean:
	rm -f mona *.o 

