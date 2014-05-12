// written by nick welch <nick@incise.org>.  author disclaims copyright.

#ifndef NUM_POINTS
#define NUM_POINTS 6
#endif

#ifndef NUM_SHAPES
#define NUM_SHAPES 40
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <pthread.h>

#define RANDINT(max)          (random() % max)
#define RANDDOUBLE(max)       ((random() / (double)RAND_MAX) * max)
#define ABS(val)              ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max)  ((val) < (min) ? (min) : \
                               (val) > (max) ? (max) : (val))

int height, width;
int mshape;

int lowestdiff = INT_MAX;
int teststep   = 0;
int beststep   = 0;


//////////////////////// X11 stuff ////////////////////////
#ifdef SHOWWINDOW

#include <X11/Xlib.h>

GC       gc;
Window   win;
Display  *dpy;
Pixmap   pixmap;
int      screen;

cairo_surface_t *xsurf;
cairo_t *xcr;
cairo_surface_t *test_surf;

void x_init(void)
{
	XInitThreads();
	XSetWindowAttributes attr;
	attr.background_pixmap = None;
	attr.backing_store = Always;

	if(!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Failed to open X display %s\n",
		        XDisplayName(NULL));
		exit(1);
	}
	screen = DefaultScreen(dpy);
	win    = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, width,
	                       height, 0, DefaultDepth(dpy, screen),
	                       CopyFromParent, CopyFromParent,
	                       CWBackPixmap|CWBackingStore, &attr);
	pixmap = XCreatePixmap(dpy, win, width, height,
	                       DefaultDepth(dpy, screen));
	gc     = XCreateGC(dpy, pixmap, 0, NULL);
	XSelectInput(dpy, win, ExposureMask);
	XMapWindow(dpy, win);
	XFlush(dpy);
}
#endif
//////////////////////// end X11 stuff ////////////////////////

typedef struct {
	double x, y;
} p_t; /* point_t */

typedef struct {
	double r, g, b, a;
	p_t    ps[NUM_POINTS];
} s_t; /* shape_t */

s_t dna_best[NUM_SHAPES];
s_t dna_test[NUM_SHAPES];

void draw_shape(s_t *dna, cairo_t *cr, int i)
{
	int j;
	s_t *shape = &dna[i];

	cairo_set_line_width(cr, 0);
	cairo_set_source_rgba(cr, shape->r, shape->g, shape->b, shape->a);
	cairo_move_to(cr, shape->ps[0].x, shape->ps[0].y);
	for(j = 1; j < NUM_POINTS; j++) {
		cairo_line_to(cr, shape->ps[j].x, shape->ps[j].y);
	}
	cairo_fill(cr);
}

void draw_dna(s_t *dna, cairo_t *cr)
{
	int i;
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	for(i = 0; i < NUM_SHAPES; i++)
		draw_shape(dna, cr, i);
}

void init_dna(s_t *dna)
{
	int i, j;
	for(i = 0; i < NUM_SHAPES; i++) {
		for(j = 0; j < NUM_POINTS; j++) {
		    dna[i].ps[j].x = RANDDOUBLE(width);
		    dna[i].ps[j].y = RANDDOUBLE(height);
		}
		dna[i].r = RANDDOUBLE(1);
		dna[i].g = RANDDOUBLE(1);
		dna[i].b = RANDDOUBLE(1);
		dna[i].a = RANDDOUBLE(1);
		//dna[i].r = 0.5;
		//dna[i].g = 0.5;
		//dna[i].b = 0.5;
		//dna[i].a = 1;
	}
}

int mutate(void)
{
	mshape          = RANDINT(NUM_SHAPES);
	double roulette = RANDDOUBLE(2.8);
	double drastic  = RANDDOUBLE(2);

	// mutate color
	if(roulette < 1) {
		// completely transparent shapes are stupid
		if(dna_test[mshape].a < 0.01 || roulette<0.25) {
			if(drastic < 1) {
				dna_test[mshape].a += RANDDOUBLE(0.1);
				/* Yay self-imposed 80 char limits!! */
				dna_test[mshape].a =
					CLAMP(dna_test[mshape].a, 0.0, 1.0);
			} else {
				dna_test[mshape].a = RANDDOUBLE(1.0);
			}
		} else if(roulette < 0.50) {
			if(drastic < 1) {
				dna_test[mshape].r += RANDDOUBLE(0.1);
				dna_test[mshape].r = 
					CLAMP(dna_test[mshape].r, 0.0, 1.0);
			} else {
				dna_test[mshape].r = RANDDOUBLE(1.0);
			}
		} else if(roulette < 0.75) {
			if(drastic < 1) {
				dna_test[mshape].g += RANDDOUBLE(0.1);
				dna_test[mshape].g =
					CLAMP(dna_test[mshape].g, 0.0, 1.0);
			} else {
				dna_test[mshape].g = RANDDOUBLE(1.0);
			}
		} else {
			if(drastic < 1) {
				dna_test[mshape].b += RANDDOUBLE(0.1);
				dna_test[mshape].b = 
					CLAMP(dna_test[mshape].b, 0.0, 1.0);
			} else {
			dna_test[mshape].b = RANDDOUBLE(1.0);
			}
		}
	}
    
    // mutate shape
	else if(roulette < 2.0) {
		int p_i = RANDINT(NUM_POINTS);
		if(roulette < 1.5) {
			if(drastic < 1) {
				dna_test[mshape].ps[p_i].x +=
				 (int)RANDDOUBLE(width / 10.0);
				dna_test[mshape].ps[p_i].x =
				 CLAMP(dna_test[mshape].ps[p_i].x, 0,
				       width - 1);
			} else {
				dna_test[mshape].ps[p_i].x = RANDDOUBLE(width);
			}
		} else {
			if(drastic < 1) {
				dna_test[mshape].ps[p_i].y +=
				 (int)RANDDOUBLE(height / 10.0);
				dna_test[mshape].ps[p_i].y =
				 CLAMP(dna_test[mshape].ps[p_i].y, 0,
				       height - 1);
			} else {
				dna_test[mshape].ps[p_i].y =
				 RANDDOUBLE(height);
			}
		}
    // mutate stacking
	} else {
		int destination = RANDINT(NUM_SHAPES);
		s_t s = dna_test[mshape];
		dna_test[mshape] = dna_test[destination];
		dna_test[destination] = s;
		return destination;
	}
	return -1;

}

int MAX_FITNESS = -1;

unsigned char *goal_data = NULL;

int difference(cairo_surface_t *test_surf, cairo_surface_t *goal_surf)
{
	int x, y;
	unsigned char *test_data = cairo_image_surface_get_data(test_surf);

	if(!goal_data) {
		goal_data = cairo_image_surface_get_data(goal_surf);
	}
	int difference = 0;
	int my_max_fitness = 0;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			int thispixel = (y * width * 4) + (x * 4);

			unsigned char test_a = test_data[thispixel];
			unsigned char test_r = test_data[thispixel + 1];
			unsigned char test_g = test_data[thispixel + 2];
			unsigned char test_b = test_data[thispixel + 3];

			unsigned char goal_a = goal_data[thispixel];
			unsigned char goal_r = goal_data[thispixel + 1];
			unsigned char goal_g = goal_data[thispixel + 2];
			unsigned char goal_b = goal_data[thispixel + 3];

			if(MAX_FITNESS == -1) {
				my_max_fitness += 
				 goal_a + goal_r + goal_g + goal_b;
			}

			difference += ABS(test_a - goal_a);
			difference += ABS(test_r - goal_r);
			difference += ABS(test_g - goal_g);
			difference += ABS(test_b - goal_b);
		}
	}

	if(MAX_FITNESS == -1)
	MAX_FITNESS = my_max_fitness;
	return difference;
}

void copy_surf_to(cairo_surface_t *surf, cairo_t *cr)
{
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	//cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, surf, 0, 0);
	cairo_paint(cr);
}

void printstats(cairo_surface_t *xsurf)
{
	struct timeval start;
	gettimeofday(&start, NULL);
	for(;;) {
#ifdef TIMELIMIT
		struct timeval t;
		gettimeofday(&t, NULL);
		if(t.tv_sec - start.tv_sec > TIMELIMIT) {
			printf("%0.6f\n",
			       ((MAX_FITNESS - lowestdiff) /
				 (float)MAX_FITNESS) * 100);
			exit(0);
		}
#ifdef DUMP
		char filename[50];
		sprintf(filename, "%d.data", getpid());
		FILE *f = fopen(filename, "w");
		fwrite(dna_best, sizeof(s_t), NUM_SHAPES, f);
		fclose(f);
#endif
#else
		printf("Step = %d/%d\nFitness = %0.6f%%\n",
		       beststep, teststep,
		       ((MAX_FITNESS - lowestdiff)
			 / (float)MAX_FITNESS) * 100);
#endif
#ifdef SHOWWINDOW
		XEvent xev;
		if(XPending(dpy) && XNextEvent(dpy, &xev) && xev.type == Expose) {
		XLockDisplay(dpy);
			XCopyArea(dpy, pixmap, win, gc, xev.xexpose.x,
			          xev.xexpose.y, xev.xexpose.width,
			          xev.xexpose.height, xev.xexpose.x,
			          xev.xexpose.y);
		XUnlockDisplay(dpy);
		}
#endif
	sleep(1);
	}
}

static void mainloop(cairo_surface_t *pngsurf)
{
	pthread_t stats_thread;

#ifdef SHOWWINDOW
	xsurf =
	 cairo_xlib_surface_create(dpy, pixmap, DefaultVisual(dpy, screen),
	                           width, height);
	xcr = cairo_create(xsurf);
#endif

	test_surf =
	 cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t *test_cr = cairo_create(test_surf);

	cairo_surface_t *goalsurf =
	 cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

	cairo_t *goalcr = cairo_create(goalsurf);
	copy_surf_to(pngsurf, goalcr);

	init_dna(dna_best);
	memcpy((void *)dna_test, (const void *)dna_best,
	       sizeof(s_t) * NUM_SHAPES);

	pthread_create(&stats_thread, NULL, (void*)printstats, (void*)NULL);

	for(;;) {
		int other_mutated = mutate();
		draw_dna(dna_test, test_cr);

		int diff = difference(test_surf, goalsurf);
		if(diff < lowestdiff) {
			beststep++;
			// test is good, copy to best
			dna_best[mshape] = dna_test[mshape];
			if(other_mutated >= 0) {
				dna_best[other_mutated] =
				 dna_test[other_mutated];
			}
			lowestdiff = diff;
			XLockDisplay(dpy);
			copy_surf_to(test_surf, xcr); // also copy to display
			XCopyArea(dpy, pixmap, win, gc, 0, 0, width, height, 0, 0);
			XFlush(dpy);
			XUnlockDisplay(dpy);
		} else {
			// test sucks, copy best back over test
			dna_test[mshape] = dna_best[mshape];
			if(other_mutated >= 0) {
				dna_test[other_mutated] =
				 dna_best[other_mutated];
			}
		}
		teststep++;
	}
}

int main(int argc, char **argv) {
	cairo_surface_t *pngsurf;

	if(argc == 1) {
		pngsurf = cairo_image_surface_create_from_png("mona.png");
	} else {
		if(access(argv[1], F_OK|R_OK)) {
			perror(argv[1]);
			return 1;
		}
		pngsurf = cairo_image_surface_create_from_png(argv[1]);
	}

	width = cairo_image_surface_get_width(pngsurf);
	height = cairo_image_surface_get_height(pngsurf);

	srandom(getpid() + time(NULL));
	x_init();
	mainloop(pngsurf);

	return 0;
}
