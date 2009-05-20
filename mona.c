// mona.c - png -> svg via genetic algorithm
// Copyright (c) 2009 David Salamon <dls@lithp.org>

/*
 *        Redistribution and use in source and binary
 * forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the
 *    above copyright notice, this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the
 *    above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation
 *    and/or other materials provided with the
 *    distribution.
 * 3. The name of the author may not be used to endorse
 *    or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// special thanks to nick welch <nick@incise.org> for the cairo & X11 glue code

typedef unsigned long long int uint64_t;

// the following macro is copyright Jay Freeman (Saurik) under the above license
#define MENES_RDTSC(x) \
    __asm__ volatile ("rdtsc" : "=A" (x)); \

#ifndef NUM_POINTS
#define NUM_POINTS 3
#endif
#ifndef NUM_SHAPES
#define NUM_SHAPES 50
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

#include <iostream>
#include <fstream>

#include <cairo.h>
#include <cairo-xlib.h>

#define RANDINT(max) (int)((random() / (double)RAND_MAX) * (max))
#define RANDOFFSET(max) (((random() / (double)RAND_MAX) * max) - (max / 2))
#define RANDDOUBLE(max) ((random() / (double)RAND_MAX) * max)
#define ABS(val) ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max) ((val) < (min) ? (min) :		\
                              (val) > (max) ? (max) : (val))

size_t WIDTH;
size_t HEIGHT;

//////////////////////// X11 stuff ////////////////////////
#ifdef SHOWWINDOW

#include <X11/Xlib.h>

Display * dpy;
int screen;
Window win;
GC gc;
Pixmap pixmap;

void x_init(void) {
  if(!(dpy = XOpenDisplay(NULL))) {
    fprintf(stderr, "Failed to open X display %s\n", XDisplayName(NULL));
    exit(1);
  }

  screen = DefaultScreen(dpy);

  XSetWindowAttributes attr;
  attr.background_pixmap = ParentRelative;
  win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0,
		      WIDTH, HEIGHT, 0,
		      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
		      CWBackPixmap, &attr);

  pixmap = XCreatePixmap(dpy, win, WIDTH, HEIGHT,
			 DefaultDepth(dpy, screen));

  gc = XCreateGC(dpy, pixmap, 0, NULL);

  XSelectInput(dpy, win, ExposureMask);

  XMapWindow(dpy, win);
}
#endif
//////////////////////// end X11 stuff ////////////////////////

struct point_t {
  size_t x, y; // 0 - (MAX_(height/width) - 1)
};

struct shape_t {
  unsigned char r, g, b; // 0 - 255
  double a; // 0 < a < 1
  point_t points[NUM_POINTS];
};

shape_t dna_best[NUM_SHAPES];
shape_t dna_test[NUM_SHAPES];

size_t offset_color_by(unsigned char a, int max) {
  int offset = ((random() / (double)RAND_MAX) * max) - (max / 2);
  if(offset < 0 && abs(offset) > a)
    return 0;
  if(offset > 0 && (offset + a) < a)
    return 255;
  return a + offset;
}

size_t offset_int_with_bounds;

void dump_best() {
  //  char filename[50];
  //  sprintf(filename, "%d.data", getpid());

  std::ofstream f;
  f.open("best.svg");
  f << "<svg xmlns=\"http://www.w3.org/2000/svg\">";
  for(size_t i=0; i!=NUM_SHAPES; ++i) {
    f << "<polygon fill=\"rgb(" << (int)dna_best[i].r << ", " << (int)dna_best[i].g << ", " << (int)dna_best[i].b << ")\" opacity=\"" << dna_best[i].a << "\" points=\"";
    for(size_t j=0; j!=NUM_POINTS; ++j)
      f << dna_best[i].points[j].x << " " << dna_best[i].points[j].y << " ";
    f << "\" />";
  }
  f << "</svg>";
  f.close();
}

void handle_signal(int signum) {
  dump_best();
  exit(0);
}

void draw_shape(shape_t * dna, cairo_t * cr, int i)
{
  cairo_set_line_width(cr, 0);
  shape_t * shape = &dna[i];
  cairo_set_source_rgba(cr, shape->r / 255.0, shape->g / 255.0, shape->b / 255.0, shape->a);
  cairo_move_to(cr, shape->points[0].x, shape->points[0].y);
  for(int j = 1; j < NUM_POINTS; j++)
    cairo_line_to(cr, shape->points[j].x, shape->points[j].y);
  cairo_fill(cr);
}

void draw_dna(shape_t * dna, cairo_t * cr)
{
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
  cairo_fill(cr);
  for(int i = 0; i < NUM_SHAPES; i++)
    draw_shape(dna, cr, i);
}

void init_dna(shape_t * dna)
{
  for(int i = 0; i < NUM_SHAPES; i++) {
    for(int j = 0; j < NUM_POINTS; j++) {
      dna[i].points[j].x = RANDINT(WIDTH);
      dna[i].points[j].y = RANDINT(HEIGHT);
    }
    dna[i].r = RANDINT(255);
    dna[i].g = RANDINT(255);
    dna[i].b = RANDINT(255);
    dna[i].a = RANDDOUBLE(1);
  }
}

uint64_t lowestdiff = INT_MAX;
uint64_t highestdiff = INT_MAX;
int teststep = 0;
int beststep = 0;

void mutate() {
  // psydo simulated annealing ->
  // make the number and size of the mutations perportional to the fittness
//  size_t mutations_to_do = highestdiff / lowestdiff;
//  mutations_to_do = (mutations_to_do >= 20 ? 1 : 21 - mutations_to_do);
  size_t mutations_to_do = 2;

  for(size_t mutation_n=0; mutation_n!=mutations_to_do; ++mutation_n) {

    size_t mutated_shape = RANDINT(NUM_SHAPES);

    // 0-49: color, 50-99: shape, 100-115: stacking
    size_t mutation_kind = RANDINT(115);

    // mutate color
    if(mutation_kind<50) {
      dna_test[mutated_shape].r = offset_color_by(dna_test[mutated_shape].r, 8);
      dna_test[mutated_shape].g = offset_color_by(dna_test[mutated_shape].g, 8);
      dna_test[mutated_shape].b = offset_color_by(dna_test[mutated_shape].b, 8);
      dna_test[mutated_shape].a = CLAMP(dna_test[mutated_shape].a + RANDOFFSET(0.1), 0.1, 1.0);
    }

    // mutate shape
    else if(mutation_kind<100) {
      //int point_i = RANDINT(NUM_POINTS);
      for(int point_i = 0; point_i!=NUM_POINTS; ++point_i) {
	dna_test[mutated_shape].points[point_i].x += (int)RANDOFFSET(WIDTH/10.0);
	dna_test[mutated_shape].points[point_i].x = CLAMP(dna_test[mutated_shape].points[point_i].x, 0, WIDTH-1);

	dna_test[mutated_shape].points[point_i].y += (int)RANDOFFSET(HEIGHT/10.0);
	dna_test[mutated_shape].points[point_i].y = CLAMP(dna_test[mutated_shape].points[point_i].y, 0, HEIGHT-1);
      }
    }

    // mutate stacking
    else {
      int destination = RANDINT(NUM_SHAPES);
      shape_t s = dna_test[mutated_shape];
      dna_test[mutated_shape] = dna_test[destination];
      dna_test[destination] = s;
    }
  }
}

uint64_t t1=0, t2=0, t3=0, pit_time=0, ptime_s=0, ptime_e=0;
bool point_in_triangle(size_t i, size_t y, size_t x) {
  MENES_RDTSC(ptime_s);
#define X(n) dna_test[i].points[(n)].x
#define Y(n) dna_test[i].points[(n)].y
  double abc_area, ab_area, ac_area, bc_area;
  abc_area = abs(X(0)*Y(1)+X(1)*Y(2)+X(2)*Y(0)-X(0)*Y(2)-X(2)*Y(1)-X(1)*Y(0))/2;
  ab_area = abs(X(0)*Y(1)+X(1)*y+x*Y(0)-X(0)*y-x*Y(1)-X(1)*Y(0))/2;
  ac_area = abs(X(0)*y+x*Y(2)+X(2)*Y(0)-X(0)*Y(2)-X(2)*y-x*Y(0))/2;
  bc_area = abs(x*Y(1)+X(1)*Y(2)+X(2)*y-x*Y(2)-X(2)*Y(1)-X(1)*y)/2;

  bool tr = abs(abc_area - (ab_area + ac_area + bc_area)) < 0.000000000001;
  MENES_RDTSC(ptime_e);
  pit_time += ptime_e - ptime_s;
  return tr;

//  A=(x1,y1) B=(x2,y2), C=(x3,y3)
//  Area= abs(x1*y2+x2*y3+x3*y1-x1*y3-x3*y2-x2*y1)/2
//  Area PAB+Area PBC +Area PAC=Area ABC

  // barycentric solution...
  // see http://www.blackpawn.com/texts/pointinpoly/default.html

  // compute vectors
  point_t v0, v1, v2;
  v0.x = dna_test[i].points[2].x - dna_test[i].points[0].x;
  v0.y = dna_test[i].points[2].y - dna_test[i].points[0].y;
  v1.x = dna_test[i].points[1].x - dna_test[i].points[0].x;
  v1.y = dna_test[i].points[1].y - dna_test[i].points[0].y;
  v2.x = x - dna_test[i].points[0].x;
  v2.y = y - dna_test[i].points[0].y;

  // compute dot products
  size_t dot00 = v0.x * v0.x + v0.y * v0.y;
  size_t dot01 = v0.x * v1.x + v0.y * v1.y;
  size_t dot02 = v0.x * v2.x + v0.y * v2.y;
  size_t dot11 = v1.x * v1.x + v1.y * v1.y;
  size_t dot12 = v1.x * v2.x + v1.y * v2.y;

  // Compute barycentric coordinates
  double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  // Check if point is in triangle
  bool result = (u > 0) && (v > 0) && (u + v < 1);
  return result;
}

unsigned char * goal_data = NULL;

template <typename T>
T min(T a, T b) { return (a > b) ? b : a; }

template <typename T>
T max(T a, T b) { return (a > b) ? a : b; }

void pull_colour(cairo_surface_t * goal_surf, size_t max_terations) {
  if(!goal_data)
    goal_data = cairo_image_surface_get_data(goal_surf);

  bool made_change = true;

  for(size_t iterct = 0; made_change && iterct != max_terations; ++iterct) {
    made_change = false;

    for(int i = 0; i!=NUM_SHAPES; ++i) {
      if(max_terations < 10)
	i = RANDINT(NUM_SHAPES);
      // WARNING: the following code only works for triangles
      size_t minx = min(min(dna_test[i].points[0].x, dna_test[i].points[1].x), dna_test[i].points[2].x);
      size_t maxx = max(max(dna_test[i].points[0].x, dna_test[i].points[1].x), dna_test[i].points[2].x);

      size_t miny = min(min(dna_test[i].points[0].y, dna_test[i].points[1].y), dna_test[i].points[2].y);
      size_t maxy = max(max(dna_test[i].points[0].y, dna_test[i].points[1].y), dna_test[i].points[2].y);

      // zero votes
      size_t votes[3][3];
      for(int vi=0; vi!=3; ++vi)
	for(int vj=0; vj!=3; ++vj)
	  votes[vi][vj] = 0;

      for(size_t y = miny; y <= maxy; y++) {
	for(size_t x = minx; x <= maxx; x++) {
	  if(! point_in_triangle(i, x,y))
	    continue;

	  // render just this pixel... three ways: base, plus, minus
	  double pixel[3][4];
	  for(int ti=0; ti!=3; ++ti)
	    for(int tj=0; tj!=4; ++tj)
	      pixel[ti][tj] = 0.0;
	  for(int j=0; j!=i; ++j) {
	    if(! point_in_triangle(j, x,y))
	      continue;
	    if(i!=j) {
	      for(int ti=0; ti!=3; ++ti) {
		for(int tj=0; tj!=4; ++tj)
		  pixel[ti][tj] *= (1.0 - dna_test[j].a);
		pixel[ti][0] += dna_test[j].a * dna_test[j].a;
		pixel[ti][1] += dna_test[j].a * dna_test[j].r;
		pixel[ti][2] += dna_test[j].a * dna_test[j].g;
		pixel[ti][3] += dna_test[j].a * dna_test[j].b;
	      }
	    }
	    else {
	      for(int ti=0; ti!=3; ++ti) {
		for(int tj=0; tj!=4; ++tj)
		  pixel[ti][tj] *= (1.0 - dna_test[j].a);
		pixel[ti][0] += dna_test[j].a * dna_test[j].a;
	      }
	      pixel[0][1] += dna_test[j].a * dna_test[j].r;
	      pixel[0][2] += dna_test[j].a * dna_test[j].g;
	      pixel[0][3] += dna_test[j].a * dna_test[j].b;

	      pixel[1][1] += dna_test[j].a * (dna_test[j].r + 1);
	      pixel[1][2] += dna_test[j].a * (dna_test[j].g + 1);
	      pixel[1][3] += dna_test[j].a * (dna_test[j].b + 1);

	      pixel[2][1] += dna_test[j].a * (dna_test[j].r - 1);
	      pixel[2][2] += dna_test[j].a * (dna_test[j].g - 1);
	      pixel[2][3] += dna_test[j].a * (dna_test[j].b - 1);
	    }
	  }

	  int thispixel = y*WIDTH*4 + x*4;
	  for(int vote_idx = 0; vote_idx!=3; ++vote_idx) {
	    unsigned char
	      goal = goal_data[thispixel + vote_idx + 1],
	      base = pixel[0][vote_idx + 1],
	      plus = pixel[1][vote_idx + 1],
	      minus = pixel[2][vote_idx + 1];

	    unsigned char
	      bd = abs(goal - base),
	      pd = abs(plus - base),
	      md = abs(minus - base);

	    if(pd < base) {
	      if(bd < md)
		++votes[vote_idx][0];
	      else
		++votes[vote_idx][2];
	    }
	    else {
	      if(pd < md)
		++votes[vote_idx][1];
	      else
		++votes[vote_idx][2];
	    }
	  }
	}
      }

      for(int ti = 0; ti!= 3; ++ti) { // r g b
	if(votes[ti][0] > votes[ti][1]) {
	  if(votes[ti][0] < votes[ti][2]) {
	    switch(i) {
	    case 0: --dna_test[ti].r; break;
	    case 1: --dna_test[ti].g; break;
	    case 2: --dna_test[ti].b; break;
	    }
	    made_change = true;
	  }
	}
	else {
	  if(votes[ti][1] > votes[ti][2]) {
	    switch(i) {
	    case 0: ++dna_test[ti].r; break;
	    case 1: ++dna_test[ti].g; break;
	    case 2: ++dna_test[ti].b; break;
	    }
	    made_change = true;
	  }
	  else {
	    switch(i) {
	    case 0: --dna_test[ti].r; break;
	    case 1: --dna_test[ti].g; break;
	    case 2: --dna_test[ti].b; break;
	    }
	    made_change = true;
	  }
	}
      }

    } // while(made_change);
  }
}


uint64_t pull_and_dist(cairo_surface_t * goal_surf) {
  if(!goal_data)
    goal_data = cairo_image_surface_get_data(goal_surf);

  /*
  unsigned char *flattened_below = alloca(sizeof(char) * HEIGHT * WIDTH * 4);
  unsigned char *flattened_above = alloca(sizeof(char) * HEIGHT * WIDTH * 4);
  for(size_t i=0; i!=(sizeof(char) * HEIGHT * WIDTH * 4); ++i)
    flattened_below[i] = 0;
  for(size_t i=0; i!=NUM_SHAPES; ++i) {
    for(size_t j=0; j!=(sizeof(char) * HEIGHT * WIDTH * 4); ++j)
      flattened_above[i] = 0;
    for(size_t j=j; j!=NUM_SHAPES; ++j) {
      for(size_t y=0; y!=HEIGHT; ++y) {
	for(size_t x=0; x!=WIDTH; ++x) {
	  size_t pix_offset = y*WIDTH*4 + x*4;
	  double opacity = cached[j][pix_offset];
	  double inv_opacity = 1.0 - cached[j][pix_offset];
	  flattened_above[pix_offset] = cached[j][pix_offset];
	  flattened_above[pix_offset + 1] *= inv_opacity;
	  flattened_above[pix_offset + 1] += opacity * cached[j][pix_offset + 1];
	  flattened_above[pix_offset + 2] *= inv_opacity;
	  flattened_above[pix_offset + 2] += mult * cached[j][pix_offset + 2];
	  flattened_above[pix_offset + 3] *= inv_opacity;
	  flattened_above[pix_offset + 3] += mult * cached[j][pix_offset + 3];
	}
      }
    }
    dna_test[i].r,g,b,a = modification;
    dna_test[i].points[0..2].x,y = modification;
    // cached[i] = cairo_render;
    // update flattened_below
  }
  */

  uint64_t difference = 0;

  /*
  for(size_t y = 0; y < HEIGHT; y++) {
    for(size_t x = 0; x < WIDTH; x++) {
      size_t pix_idx = y*WIDTH*4 + x*4;
      for(size_t i=0; i!=4; ++i) {
       	int delta = flattened_below[pix_idx + i] - goal_data[pix_idx + i];
	difference += delta * delta;
      }
    }
  }
  */

  return difference;
}

uint64_t difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf) {
  if(!goal_data)
    goal_data = cairo_image_surface_get_data(goal_surf);
  unsigned char * test_data = cairo_image_surface_get_data(test_surf);

  uint64_t difference = 0;

  for(size_t y = 0; y < HEIGHT; y++) {
    for(size_t x = 0; x < WIDTH; x++) {
      size_t pix_idx = y*WIDTH*4 + x*4;
      for(size_t i=0; i!=4; ++i) {
	int delta = test_data[pix_idx + i] - goal_data[pix_idx + i];
	difference += delta * delta;
      }
    }
  }

  return difference;
}

void copy_surf_to(cairo_surface_t * surf, cairo_t * cr)
{
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
  cairo_fill(cr);
  //cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(cr, surf, 0, 0);
  cairo_paint(cr);
}

uint64_t show_time=0, test_time=0, mutate_time=0, color_time=0, loop_time=0;
static void mainloop(cairo_surface_t * pngsurf)
{
  struct timeval start;
  gettimeofday(&start, NULL);

  init_dna(dna_best);
  memcpy((void *)dna_test, (const void *)dna_best, sizeof(shape_t) * NUM_SHAPES);

#ifdef SHOWWINDOW
  cairo_surface_t * xsurf = cairo_xlib_surface_create(
						      dpy, pixmap, DefaultVisual(dpy, screen), WIDTH, HEIGHT);
  cairo_t * xcr = cairo_create(xsurf);
#endif

  cairo_surface_t * test_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
  cairo_t * test_cr = cairo_create(test_surf);

  cairo_surface_t * goalsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
  cairo_t * goalcr = cairo_create(goalsurf);
  copy_surf_to(pngsurf, goalcr);

  for(;;) {
    MENES_RDTSC(t1);
    //    if((teststep != 0) && (teststep % 1000 == 0))
    //      pull_colour(goalsurf, 100);
    //    else {
    {
      mutate();
      MENES_RDTSC(t2);
      //pull_colour(goalsurf, 1);
      MENES_RDTSC(t3);
      mutate_time += t2 - t1;
      color_time += t3 - t2;
    }

    draw_dna(dna_test, test_cr);
    uint64_t diff = difference(test_surf, goalsurf);
    if(highestdiff == INT_MAX)
      highestdiff = diff;

    MENES_RDTSC(t2); test_time += t2 - t1;

//    std::cout << "diff: " << diff << "\tdiff2: " << dist(goalsurf) << std::endl;

    if(diff < lowestdiff) {         // test is good, copy to best
      beststep++;
      for(int i=0; i!=NUM_SHAPES; ++i)
	dna_best[i] = dna_test[i];
      lowestdiff = diff;

#ifdef SHOWWINDOW
      copy_surf_to(test_surf, xcr); // also copy to display
      XCopyArea(dpy, pixmap, win, gc,
		0, 0,
		WIDTH, HEIGHT,
		0, 0);
#endif
    }
    else {
      // test sucks, copy best back over test
      for(int i=0; i!=NUM_SHAPES; ++i)
	dna_test[i] = dna_best[i];
    }

    teststep++;

    if(teststep % 100 == 0) {
      std::cout << "HEIGHT: " << HEIGHT << "    WIDTH: " << WIDTH << std::endl;
      std::cout << "Step = " << beststep << " / " << teststep << std::endl;
      std::cout << "Fitness = " << lowestdiff << std::endl;
      std::cout << "Per pixel accuracy = " << (1.0 - (lowestdiff / ((double)(255 << 8) * HEIGHT * WIDTH))) * 100 << std::endl;
      std::cout << "Mutate time: " << mutate_time << std::endl;
      std::cout << "Color time: " << color_time << std::endl;
      std::cout << "Pit time: " << pit_time << std::endl;
      std::cout << "__Test time: " << test_time << std::endl;
      std::cout << "Loop time: " << loop_time << std::endl << std::endl;
      test_time=0; pit_time = 0; color_time = 0; mutate_time = 0; loop_time = 0;
    }

#ifdef SHOWWINDOW
    if(teststep % 100 == 0 && XPending(dpy)) {
      XEvent xev;
      XNextEvent(dpy, &xev);
      switch(xev.type) {
      case Expose:
	XCopyArea(dpy, pixmap, win, gc,
		  xev.xexpose.x, xev.xexpose.y,
		  xev.xexpose.width, xev.xexpose.height,
		  xev.xexpose.x, xev.xexpose.y);
      }
    }
#endif

    MENES_RDTSC(t2); loop_time += t2 - t1;
  }
}

int main(int argc, char ** argv) {
  if(NUM_POINTS != 3) {
    // color pulling is currently only implemented for triangles
    std::cout << "Error -- all true shapes have exactly three points." << std::endl;
    exit(1);
  }

  signal(SIGINT, handle_signal);

  cairo_surface_t * pngsurf;
  if(argc == 1) {
    std::cout << "usage: ./mona <png name>" << std::endl;
    exit(0);
  }
  else
    pngsurf = cairo_image_surface_create_from_png(argv[1]);

  WIDTH = cairo_image_surface_get_width(pngsurf);
  HEIGHT = cairo_image_surface_get_height(pngsurf);

  srandom(getpid() + time(NULL));
  x_init();
  mainloop(pngsurf);
}
