// written by nick welch <nick@incise.org>.  author disclaims copyright.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <getopt.h>

#include <cairo.h>

#define RANDINT(max) (int)((rand() / (double)RAND_MAX) * (max))
#define RANDDOUBLE(max) ((rand() / (double)RAND_MAX) * max)
#define ABS(val) ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))

#define MAX_POINTS 16

int WIDTH;
int HEIGHT;

unsigned long TIMELIMIT = 0;
bool SHOW_WINDOW = true;
bool DUMP = false;
int REPEAT = 0;
char* OUTPUT_FILENAME = "oops.png";
int NUM_POINTS = 6;
int NUM_SHAPES = 40;

//////////////////////// X11 stuff ////////////////////////
#ifdef WITH_SDL

#include "SDL.h"

SDL_Window *win;
SDL_Surface *screen;

void x_init(void)
{
	if (SHOW_WINDOW) {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			fprintf(stderr, "ERROR: %s\n", SDL_GetError());
			exit(1);
		}
		win = SDL_CreateWindow("mona",
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);

		if (win == NULL) {
			fprintf(stderr, "ERROR: %s\n", SDL_GetError());
			exit(1);
		}

		screen = SDL_GetWindowSurface(win);
	}
}

#else
void x_init(void)
{
}
#endif
//////////////////////// end X11 stuff ////////////////////////

typedef struct {
	double x, y;
} point_t;

typedef struct {
	double r, g, b, a;
	point_t points[MAX_POINTS];
} shape_t;

int mutated_shape;

void draw_shape(shape_t * dna, cairo_t * cr, int i)
{
	cairo_set_line_width(cr, 0);
	shape_t *shape = &dna[i];
	cairo_set_source_rgba(cr, shape->r, shape->g, shape->b, shape->a);
	cairo_move_to(cr, shape->points[0].x, shape->points[0].y);
	for (int j = 1; j < NUM_POINTS; j++)
		cairo_line_to(cr, shape->points[j].x, shape->points[j].y);
	cairo_fill(cr);
}

void draw_dna(shape_t * dna, cairo_t * cr)
{
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
	cairo_fill(cr);
	for (int i = 0; i < NUM_SHAPES; i++)
		draw_shape(dna, cr, i);
}

void init_dna(shape_t * dna)
{
	for (int i = 0; i < NUM_SHAPES; i++) {
		for (int j = 0; j < NUM_POINTS; j++) {
			dna[i].points[j].x = RANDDOUBLE(WIDTH);
			dna[i].points[j].y = RANDDOUBLE(HEIGHT);
		}
		dna[i].r = RANDDOUBLE(1);
		dna[i].g = RANDDOUBLE(1);
		dna[i].b = RANDDOUBLE(1);
		dna[i].a = RANDDOUBLE(1);
	}
}

int mutate(shape_t* dna_test)
{
	mutated_shape = RANDINT(NUM_SHAPES);
	double roulette = RANDDOUBLE(2.8);
	double drastic = RANDDOUBLE(2);

	// mutate color
	if (roulette < 1) {
		if (dna_test[mutated_shape].a < 0.01	// completely transparent shapes are stupid
				|| roulette < 0.25) {
			if (drastic < 1) {
				dna_test[mutated_shape].a += RANDDOUBLE(0.1);
				dna_test[mutated_shape].a =
					CLAMP(dna_test[mutated_shape].a, 0.0, 1.0);
			} else
				dna_test[mutated_shape].a = RANDDOUBLE(1.0);
		} else if (roulette < 0.50) {
			if (drastic < 1) {
				dna_test[mutated_shape].r += RANDDOUBLE(0.1);
				dna_test[mutated_shape].r =
					CLAMP(dna_test[mutated_shape].r, 0.0, 1.0);
			} else
				dna_test[mutated_shape].r = RANDDOUBLE(1.0);
		} else if (roulette < 0.75) {
			if (drastic < 1) {
				dna_test[mutated_shape].g += RANDDOUBLE(0.1);
				dna_test[mutated_shape].g =
					CLAMP(dna_test[mutated_shape].g, 0.0, 1.0);
			} else
				dna_test[mutated_shape].g = RANDDOUBLE(1.0);
		} else {
			if (drastic < 1) {
				dna_test[mutated_shape].b += RANDDOUBLE(0.1);
				dna_test[mutated_shape].b =
					CLAMP(dna_test[mutated_shape].b, 0.0, 1.0);
			} else
				dna_test[mutated_shape].b = RANDDOUBLE(1.0);
		}
	}
	// mutate shape
	else if (roulette < 2.0) {
		int point_i = RANDINT(NUM_POINTS);
		if (roulette < 1.5) {
			if (drastic < 1) {
				dna_test[mutated_shape].points[point_i].x +=
					(int) RANDDOUBLE(WIDTH / 10.0);
				dna_test[mutated_shape].points[point_i].x =
					CLAMP(dna_test[mutated_shape].points[point_i].x, 0,
							WIDTH - 1);
			} else
				dna_test[mutated_shape].points[point_i].x =
					RANDDOUBLE(WIDTH);
		} else {
			if (drastic < 1) {
				dna_test[mutated_shape].points[point_i].y +=
					(int) RANDDOUBLE(HEIGHT / 10.0);
				dna_test[mutated_shape].points[point_i].y =
					CLAMP(dna_test[mutated_shape].points[point_i].y, 0,
							HEIGHT - 1);
			} else
				dna_test[mutated_shape].points[point_i].y =
					RANDDOUBLE(HEIGHT);
		}
	}
	// mutate stacking
	else {
		int destination = RANDINT(NUM_SHAPES);
		shape_t s = dna_test[mutated_shape];
		dna_test[mutated_shape] = dna_test[destination];
		dna_test[destination] = s;
		return destination;
	}
	return -1;

}

int MAX_FITNESS = -1;

unsigned char *goal_data = NULL;

int difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf)
{
	unsigned char *test_data = cairo_image_surface_get_data(test_surf);
	if (!goal_data)
		goal_data = cairo_image_surface_get_data(goal_surf);

	int difference = 0;

	int my_max_fitness = 0;

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			int thispixel = y * WIDTH * 4 + x * 4;

			unsigned char test_a = test_data[thispixel];
			unsigned char test_r = test_data[thispixel + 1];
			unsigned char test_g = test_data[thispixel + 2];
			unsigned char test_b = test_data[thispixel + 3];

			unsigned char goal_a = goal_data[thispixel];
			unsigned char goal_r = goal_data[thispixel + 1];
			unsigned char goal_g = goal_data[thispixel + 2];
			unsigned char goal_b = goal_data[thispixel + 3];

			if (MAX_FITNESS == -1)
				my_max_fitness += goal_a + goal_r + goal_g + goal_b;

			difference += ABS(test_a - goal_a);
			difference += ABS(test_r - goal_r);
			difference += ABS(test_g - goal_g);
			difference += ABS(test_b - goal_b);
		}
	}

	if (MAX_FITNESS == -1)
		MAX_FITNESS = my_max_fitness;
	return difference;
}

void copy_surf_to(cairo_surface_t * surf, cairo_t * cr)
{
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
	cairo_fill(cr);
	cairo_set_source_surface(cr, surf, 0, 0);
	cairo_paint(cr);
}

void write_timelapse_img(cairo_surface_t* final_surf, int frame)
{
	char fname[255];
	snprintf(fname, 255, "%.4d%s", frame, OUTPUT_FILENAME);
	cairo_status_t err = cairo_surface_write_to_png(final_surf, fname);
	if (err != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "Error writing image to disk: %s",
				cairo_status_to_string(err));
	}
	fprintf(stderr, "Image succesfully saved as '%s'\n", fname);
}

void write_img(cairo_surface_t* final_surf)
{
	cairo_status_t err = cairo_surface_write_to_png(final_surf, OUTPUT_FILENAME);
	if (err != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "Error writing image to disk: %s",
				cairo_status_to_string(err));
	}
	fprintf(stderr, "Image succesfully saved as '%s'\n", OUTPUT_FILENAME);
}

/* TODO: copy the C# dna format */
void write_dna(shape_t* dna)
{
		char fname[255]; // FIXME
		strcpy(fname, OUTPUT_FILENAME);
		strcat(fname, ".dna");

		FILE *f = fopen(fname, "w");
		fwrite(dna, sizeof(shape_t), NUM_SHAPES, f);
		fclose(f);
}

static void mainloop(cairo_surface_t * pngsurf)
{

	shape_t dna_best[NUM_SHAPES];
	shape_t dna_test[NUM_SHAPES];

	struct timeval start;
	gettimeofday(&start, NULL);

#ifdef WITH_SDL
	cairo_surface_t *xsurf = NULL;
	cairo_t *xcr = NULL;
	if (SHOW_WINDOW) {
		xsurf = cairo_image_surface_create_for_data(screen->pixels,
				CAIRO_FORMAT_RGB24,
				WIDTH, HEIGHT,
				screen->pitch);

		xcr = cairo_create(xsurf);
	}
#endif

	init_dna(dna_best);
	memcpy((void *) dna_test, (const void *) dna_best,
			sizeof(shape_t) * NUM_SHAPES);


	cairo_surface_t *test_surf =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
	cairo_t *test_cr = cairo_create(test_surf);

	cairo_surface_t *goalsurf =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
	cairo_t *goalcr = cairo_create(goalsurf);
	copy_surf_to(pngsurf, goalcr);

	int lowestdiff = INT_MAX;
	int teststep = 0;
	int lapses   = 0;
	int beststep = 0;
	for (;;) {
#ifdef WITH_SDL
		if (SHOW_WINDOW) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT
						|| event.type == SDL_WINDOWEVENT_CLOSE) {
					goto cleanup;
				}
			}
		}
#endif
		int other_mutated = mutate(dna_test);
		draw_dna(dna_test, test_cr);

		int diff = difference(test_surf, goalsurf);
		if (diff < lowestdiff) {
			beststep++;
			// test is good, copy to best
			dna_best[mutated_shape] = dna_test[mutated_shape];
			if (other_mutated >= 0)
				dna_best[other_mutated] = dna_test[other_mutated];
#ifdef WITH_SDL
			if (SHOW_WINDOW) {
				copy_surf_to(test_surf, xcr);	// also copy to display
				SDL_UpdateWindowSurface(win);
			}
#endif
			lowestdiff = diff;
		} else {
			// test sucks, copy best back over test
			dna_test[mutated_shape] = dna_best[mutated_shape];
			if (other_mutated >= 0)
				dna_test[other_mutated] = dna_best[other_mutated];
		}

		teststep++;
		if (teststep % 100 == 0) {
			printf("Step = %d/%d\nFitness = %0.6f%%\n",
					beststep, teststep,
					((MAX_FITNESS -
					  lowestdiff) / (float) MAX_FITNESS) * 100);
		}

		if (REPEAT != 0 && beststep % REPEAT == 0) {
			write_timelapse_img(test_surf, lapses);
			lapses++;
		}


		if (TIMELIMIT != 0) {
			struct timeval t;
			gettimeofday(&t, NULL);
			if (t.tv_sec - start.tv_sec > TIMELIMIT) {
				printf("%0.6f\n",
						((MAX_FITNESS -
						  lowestdiff) / (float) MAX_FITNESS) * 100);
				goto cleanup;
			}
		}
	}

cleanup:
	if (DUMP) {
		write_img(test_surf);
		write_dna(dna_best);
	}
	return;
}


void print_help(const char *program)
{
	printf(
"Usage: %s [OPTION]... [FILE]\n"
"Evolves an image out of triangles from a source image, using simmulated\n"
"  annealing. Input images must be in the PNG format.\n"
"\n"
" -n        hides the intermediate view, such that there is (N)o window.\n"
"             Images will continue to be generated in the background.\n"
" -t TIME   Sets a time limit in seconds. Program will terminate afterward.\n"
" -r ITER   Timelapse mode: will dump intermediate images every ITER iterations.\n"
" -o FILE   Outputs most fit image at the end of the time limit, named FILE.\n"
" -s NUM    The generated images will be made up of NUM polygons\n"
" -p NUM    The polygons making up the image will have NUM vertices.\n"
"             Three or more, up to 16.\n"
" -h        Displays this help and exits.\n"
	, program);
}

void print_config(const char* input)
{
	fprintf(stderr, "Input from %s\n", input);
	if (DUMP)
		fprintf(stderr, "Final output at '%s', dna at '%s.dna'\n",
				OUTPUT_FILENAME, OUTPUT_FILENAME);

	if (TIMELIMIT)
		fprintf(stderr, "Running for %lu seconds.\n", TIMELIMIT);
	fprintf(stderr, "Images will have %d shapes.\n", NUM_SHAPES);
	fprintf(stderr, "Shapes will have %d points.\n", NUM_POINTS);
}

int main(int argc, char **argv)
{
	cairo_surface_t *pngsurf;
	char* input_name = "mona.png";
	char* timestr = NULL;
	char c;

	/* FIXME do proper error checking on the arguments */
	while ((c = getopt(argc, argv, "rntopsh")) != -1) {
		switch (c) {
			case 'o':
				DUMP = true;
				OUTPUT_FILENAME = argv[optind++];
				break;
			case 't':
				timestr = argv[optind++];
				TIMELIMIT = strtol(timestr, NULL, 10);
				if (TIMELIMIT == 0) {
					fprintf(stderr, "ERROR: Not a valid time: '%s'\n", timestr);
					return 1;
				}
				break;
			case 'n':
				SHOW_WINDOW = false;
				break;
			case 'r':
				REPEAT = strtol(argv[optind++], NULL, 10);
				break;
			case 's':
				NUM_SHAPES = strtol(argv[optind++], NULL, 10);
				break;
			case 'p':
				NUM_POINTS = strtol(argv[optind++], NULL, 10);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
			default:
				fprintf(stderr, "ERROR: unrecognized argument\n");
				return 1;
		}
	}

	if (argc - optind == 1) {
		input_name = argv[optind];
	}

	print_config(input_name);

	pngsurf = cairo_image_surface_create_from_png(input_name);
	if (cairo_surface_status(pngsurf) != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "ERROR: '%s' is not a valid png file (probably).\n",
				input_name);
		return 1;
	}

	WIDTH = cairo_image_surface_get_width(pngsurf);
	HEIGHT = cairo_image_surface_get_height(pngsurf);

	srand(getpid() + time(NULL));
	x_init();
	mainloop(pngsurf);
}
