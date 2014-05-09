// written by nick welch <nick@incise.org>.  author disclaims copyright.

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

#include <cairo.h>
#include "mona.h"
#include "diff.h"

#define MAX_POINTS 16

int WIDTH, HEIGHT;

unsigned long TIMELIMIT = 0;
bool SHOW_WINDOW = true;
bool DUMP = false;
int REPEAT = 0;
int SESSION_REPEAT = 0;
char* OUTPUT_FILENAME = "oops.png";
int NUM_POINTS = 6;
int NUM_SHAPES = 40;

//////////////////////// X11 stuff ////////////////////////
#ifdef WITH_SDL

#include "SDL.h"

SDL_Window *win;
SDL_Surface *screen;

void x_init(const char* name)
{
	if (SHOW_WINDOW) {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			fprintf(stderr, "ERROR: %s\n", SDL_GetError());
			exit(1);
		}
		win = SDL_CreateWindow(name,
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);

		if (win == NULL) {
			fprintf(stderr, "ERROR: %s\n", SDL_GetError());
			exit(1);
		}

		screen = SDL_GetWindowSurface(win);
	}
}

void x_clean(void)
{
	SDL_FreeSurface(screen);
	SDL_DestroyWindow(win);
	SDL_Quit();
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
void write_dna(const char * filename, const shape_t * dna)
{
		// FIXME: Unsafe usage of fname and fname_new
		char fname_new[255];
		strcpy(fname_new, filename);
		strcat(fname_new, ".dna.new");

		FILE *f = fopen(fname_new, "w");
		fwrite(dna, sizeof(shape_t), NUM_SHAPES, f);
		fclose(f);

		char fname[255];
		strcpy(fname, filename);
		strcat(fname, ".dna");

		// If fname already exist, ensure that either fname or fname_new will be saved.
		rename(fname_new, fname);
}

/* FIXME: NUM_SHAPES and NUM_POINTS must match old values, maybe encode them to saved DNA? */
void read_dna(const char * filename, shape_t * dna)
{
	FILE * f = fopen(filename, "r");
	// FIXME: analyze return value
	fread(dna, sizeof(shape_t), NUM_SHAPES, f);
	fclose(f);
}

static bool running = true;


void stopLoop()
{
	running = false;
}

static void mainloop(cairo_surface_t * pngsurf, const char * input_dna_name)
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

	difference_init();

	if(input_dna_name)
		read_dna(input_dna_name, dna_best);
	else
		init_dna(dna_best);

	memcpy((void *) dna_test, (const void *) dna_best,
			sizeof(shape_t) * NUM_SHAPES);


	cairo_surface_t *test_surf =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
	cairo_t *test_cr = cairo_create(test_surf);

	cairo_surface_t *goal_surf =
		cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
	cairo_t *goalcr = cairo_create(goal_surf);
	copy_surf_to(pngsurf, goalcr);

	int lowestdiff = INT_MAX;
	int teststep = 0;
	int lapses   = 0;
	int beststep = 0;
	running = true;
	while (running) {
#ifdef WITH_SDL
		if (SHOW_WINDOW) {
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT
						|| event.type == SDL_WINDOWEVENT_CLOSE) {
					stopLoop();
				}
			}
		}
#endif
		int other_mutated = mutate(dna_test);
		draw_dna(dna_test, test_cr);

		int diff = difference(test_surf, goal_surf);
		bool good = diff < lowestdiff ;
		if (good) {
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
					((get_max_fitness() -
					  lowestdiff) / (float) get_max_fitness()) * 100);
		}

		if (REPEAT != 0 && good && beststep % REPEAT == 0) {
			write_timelapse_img(test_surf, lapses);
			lapses++;
		}

		if (SESSION_REPEAT != 0 && good && beststep % SESSION_REPEAT == 0) {
			write_img(test_surf); // assuming that test and best are equal on this step
			write_dna(OUTPUT_FILENAME, dna_best);
		}

		if (TIMELIMIT != 0) {
			struct timeval t;
			gettimeofday(&t, NULL);
			if (t.tv_sec - start.tv_sec > TIMELIMIT) {
				printf("%0.6f\n",
						((get_max_fitness() -
						  lowestdiff) / (float) get_max_fitness()) * 100);
				stopLoop();
			}
		}
	}

cleanup:
	difference_clean();
	if (DUMP) {
		write_img(test_surf);
		write_dna(OUTPUT_FILENAME, dna_best);
	}
	cairo_destroy(test_cr);
	cairo_surface_destroy(test_surf);
	cairo_destroy(goalcr);
	cairo_surface_destroy(goal_surf);
#ifdef WITH_SDL
	cairo_destroy(xcr);
	cairo_surface_destroy(xsurf);
#endif
	return;
}


void print_help(const char *program)
{
	printf(
"Usage: %s [OPTION]... [INPUT_IMAGE] [INPUT_DNA]\n"
"Evolves an image out of triangles from a source image, using simmulated\n"
"  annealing. Input images must be in the PNG format.\n"
"\n"
" -n        hides the intermediate view, such that there is (N)o window.\n"
"             Images will continue to be generated in the background.\n"
" -t TIME   Sets a time limit in seconds. Program will terminate afterward.\n"
" -r ITER   Timelapse mode: will dump intermediate images every ITER iterations.\n"
" -o FILE   Outputs most fit image and DNA at the end of the time limit to FILE.\n"
" -S ITER   Session mode: will save most fit image and DNA to file every ITER iterations. Use filename from \"-o\".\n"
" -s NUM    The generated images will be made up of NUM polygons\n"
" -p NUM    The polygons making up the image will have NUM vertices.\n"
"             Three or more, up to 16.\n"
" -g NUM    Sets the seed for the random number generator to NUM. base-10 only.\n"
" -h        Displays this help and exits.\n"
	, program);
}

void print_config(const char* input, const char* input_dna)
{
	fprintf(stderr, "Input image from %s\n", input);
	if (input_dna)
		fprintf(stderr, "Input DNA from %s\n", input_dna);
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
	const char* program_name = argv[0];
	char* input_image_name = "mona.png";
	char* input_dna_name = NULL;
	char* timestr = NULL;
	long seed = getpid() + time(NULL);
	char c;

	/* FIXME do proper error checking on the arguments */
	while ((c = getopt(argc, argv, "g:hno:S:p:r:s:t:")) != -1)
		switch (c) {
		case 'o':
			DUMP = true;
			OUTPUT_FILENAME = optarg;
			break;
		case 'S':
			SESSION_REPEAT = strtol(optarg, NULL, 10);
			break;
		case 't':
			TIMELIMIT = strtol(optarg, NULL, 10);
			if (TIMELIMIT == 0) {
				fprintf(stderr, "ERROR: Not a valid time: '%s'\n", timestr);
				return 1;
			}
			break;
		case 'n':
			SHOW_WINDOW = false;
			break;
		case 'r':
			REPEAT = strtol(optarg, NULL, 10);
			break;
		case 's':
			NUM_SHAPES = strtol(optarg, NULL, 10);
			break;
		case 'p':
			NUM_POINTS = strtol(optarg, NULL, 10);
			break;
		case 'g':
			seed = strtol(optarg, NULL, 10);
			break;
		case 'h':
			print_help(program_name);
			return 0;
		default:
			fprintf(stderr, "ERROR: unrecognized argument\n");
			return 1;
		}

	if (argc - optind >= 1) {
		input_image_name = argv[optind++];
	}

	if (argc - optind >= 1) {
		input_dna_name = argv[optind++];
	}

	print_config(input_image_name, input_dna_name);

	pngsurf = cairo_image_surface_create_from_png(input_image_name);
	if (cairo_surface_status(pngsurf) != CAIRO_STATUS_SUCCESS) {
		fprintf(stderr, "ERROR: '%s' is not a valid png file (probably).\n",
				input_image_name);
		return 1;
	}

	WIDTH = cairo_image_surface_get_width(pngsurf);
	HEIGHT = cairo_image_surface_get_height(pngsurf);

	signal(SIGINT, stopLoop);
#ifdef WITH_SDL
	x_init(program_name);
#endif
	srand(seed);
	mainloop(pngsurf, input_dna_name);
	cairo_surface_destroy(pngsurf);
#ifdef WITH_SDL
	x_clean();
#endif
}
