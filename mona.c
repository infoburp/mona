// written by nick welch <nick@incise.org>.  author disclaims copyright.

#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include "mona.h"
#include "cJSON.h"
#include "save.h"

#define RANDINT(max) (int)((random() / (double)RAND_MAX) * (max))
#define RANDDOUBLE(max) ((random() / (double)RAND_MAX) * max)
#define ABS(val) ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : \
                              (val) > (max) ? (max) : (val))

int WIDTH, HEIGHT;
int POINTS = 6;
int SHAPES = 40;
//////////////////////// X11 stuff ////////////////////////
#include <X11/Xlib.h> 

Display * dpy;
int screen;
Window win;
GC gc;
Pixmap pixmap;

void x_init(void)
{
    if (!(dpy = XOpenDisplay(NULL)))
    {
        fprintf(stderr, "Failed to open X display %s\n", XDisplayName(NULL));
        exit(1);
    }

    screen = DefaultScreen(dpy);

    Window rootwin = RootWindow(dpy, screen);
    win = XCreateSimpleWindow(dpy, rootwin, 0, 0, WIDTH, HEIGHT, 0, 
                 BlackPixel(dpy, screen), BlackPixel(dpy, screen)); 
    pixmap = XCreatePixmap(dpy, win, WIDTH, HEIGHT,
            DefaultDepth(dpy, screen));

    gc = XCreateGC(dpy, pixmap, 0, NULL);

    XSelectInput(dpy, win, ExposureMask);

    XMapWindow(dpy, win);
}
//////////////////////// end X11 stuff ////////////////////////

shape_t* dna_best = 0;
shape_t* dna_test = 0;

int mutated_shape;



void draw_shape(shape_t * dna, cairo_t * cr, int i)
{
    cairo_set_line_width(cr, 0);
    shape_t * shape = &dna[i];
    cairo_set_source_rgba(cr, shape->r, shape->g, shape->b, shape->a);
    cairo_move_to(cr, shape->points[0].x, shape->points[0].y);
    for(int j = 1; j < POINTS; j++)
        cairo_line_to(cr, shape->points[j].x, shape->points[j].y);
    cairo_fill(cr);
}

void draw_dna(shape_t * dna, cairo_t * cr)
{
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_fill(cr);
    for(int i = 0; i < SHAPES; i++)
        draw_shape(dna, cr, i);
}

shape_t* init_dna()
{
    shape_t* dna = malloc(SHAPES * sizeof(shape_t));
    for(int i = 0; i < SHAPES; i++)
    {
        dna[i].points = malloc(POINTS * sizeof(point_t));
	for(int j = 0; j < POINTS; j++)
        {
            dna[i].points[j].x = RANDDOUBLE(WIDTH);
            dna[i].points[j].y = RANDDOUBLE(HEIGHT);
        }
        dna[i].r = RANDDOUBLE(1);
        dna[i].g = RANDDOUBLE(1);
        dna[i].b = RANDDOUBLE(1);
        dna[i].a = RANDDOUBLE(1);
    }
    return dna;
}

int mutate(void)
{
    mutated_shape = RANDINT(SHAPES);
    double roulette = RANDDOUBLE(2.8);
    double drastic = RANDDOUBLE(2);
     
    // mutate color
    if (roulette<1)
    {
        if (dna_test[mutated_shape].a < 0.01 // completely transparent shapes are stupid
                || roulette<0.25)
        {
            if (drastic < 1)
            {
                dna_test[mutated_shape].a += RANDDOUBLE(0.1);
                dna_test[mutated_shape].a = CLAMP(dna_test[mutated_shape].a, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].a = RANDDOUBLE(1.0);
        }
        else if (roulette<0.50)
        {
            if (drastic < 1)
            {
                dna_test[mutated_shape].r += RANDDOUBLE(0.1);
                dna_test[mutated_shape].r = CLAMP(dna_test[mutated_shape].r, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].r = RANDDOUBLE(1.0);
        }
        else if (roulette<0.75)
        {
            if (drastic < 1)
            {
                dna_test[mutated_shape].g += RANDDOUBLE(0.1);
                dna_test[mutated_shape].g = CLAMP(dna_test[mutated_shape].g, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].g = RANDDOUBLE(1.0);
        }
        else
        {
            if (drastic < 1)
            {
                dna_test[mutated_shape].b += RANDDOUBLE(0.1);
                dna_test[mutated_shape].b = CLAMP(dna_test[mutated_shape].b, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].b = RANDDOUBLE(1.0);
        }
    }
    
    // mutate shape
    else if (roulette < 2.0)
    {
        int point_i = RANDINT(POINTS);
        if (roulette<1.5)
        {
            if (drastic < 1)
            {
                dna_test[mutated_shape].points[point_i].x += (int)RANDDOUBLE(WIDTH/10.0);
                dna_test[mutated_shape].points[point_i].x = CLAMP(dna_test[mutated_shape].points[point_i].x, 0, WIDTH-1);
            }
            else
                dna_test[mutated_shape].points[point_i].x = RANDDOUBLE(WIDTH);
        }
        else
        {
            if (drastic < 1)
            {
                dna_test[mutated_shape].points[point_i].y += (int)RANDDOUBLE(HEIGHT/10.0);
                dna_test[mutated_shape].points[point_i].y = CLAMP(dna_test[mutated_shape].points[point_i].y, 0, HEIGHT-1);
            }
            else
                dna_test[mutated_shape].points[point_i].y = RANDDOUBLE(HEIGHT);
        }
    }

    // mutate stacking
    else
    {
        int destination = RANDINT(SHAPES);
        shape_t s = dna_test[mutated_shape];
        dna_test[mutated_shape] = dna_test[destination];
        dna_test[destination] = s;
        return destination;
    }
    return -1;

}

int MAX_FITNESS = -1;

unsigned char * goal_data = NULL;

int difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf)
{
    unsigned char * test_data = cairo_image_surface_get_data(test_surf);
    if (!goal_data)
        goal_data = cairo_image_surface_get_data(goal_surf);

    int difference = 0;

    int my_max_fitness = 0;

    for(int y = 0; y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            int thispixel = y*WIDTH*4 + x*4;

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
    //cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_paint(cr);
}

void copy_shape(shape_t* src, shape_t* dst, int index)
{
    dst[index].r = src[index].r;
    dst[index].g = src[index].g;
    dst[index].b = src[index].b;
    dst[index].a = src[index].a;
    for (int j = 0; j < POINTS; j++)
    {
        dst[index].points[j].x = src[index].points[j].x;
        dst[index].points[j].y = src[index].points[j].y;
    }
}

static void mainloop(shape_t* dna, cairo_surface_t * pngsurf)
{
    struct timeval start;
    gettimeofday(&start, NULL);
    dna_test = init_dna();

    /* copy dna_best to dna_test */
    for (int i = 0; i < SHAPES; i++)
        copy_shape(dna, dna_test, i); 
    cairo_surface_t * xsurf = cairo_xlib_surface_create(
            dpy, pixmap, DefaultVisual(dpy, screen), WIDTH, HEIGHT);
    cairo_t * xcr = cairo_create(xsurf);

    cairo_surface_t * test_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t * test_cr = cairo_create(test_surf);

    cairo_surface_t * goalsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t * goalcr = cairo_create(goalsurf);
    copy_surf_to(pngsurf, goalcr);

    int lowestdiff = INT_MAX;
    int teststep = 0;
    int beststep = 0;
    for(;;) {
        int other_mutated = mutate();
        draw_dna(dna_test, test_cr);

        int diff = difference(test_surf, goalsurf);
        if (diff < lowestdiff)
        {
            beststep++;
            // test is good, copy to best
            copy_shape(dna_test, dna, mutated_shape);
            if (other_mutated >= 0)
                copy_shape(dna_test, dna, other_mutated);
            copy_surf_to(test_surf, xcr); // also copy to display
            XCopyArea(dpy, pixmap, win, gc,
                    0, 0,
                    WIDTH, HEIGHT,
                    0, 0);
            lowestdiff = diff;
        }
        else
        {
            // test sucks, copy best back over test
            copy_shape(dna, dna_test, mutated_shape);
            if (other_mutated >= 0)
                copy_shape(dna, dna_test, other_mutated);
        }

        teststep++;

#ifdef TIMELIMIT
        struct timeval t;
        gettimeofday(&t, NULL);
        if (t.tv_sec - start.tv_sec > TIMELIMIT)
        {
            printf("%0.6f\n", ((MAX_FITNESS-lowestdiff) / (float)MAX_FITNESS)*100);
#ifdef DUMP
            char filename[50];
            sprintf(filename, "%d.data", getpid());
            FILE * f = fopen(filename, "w");
            fwrite(dna_best, sizeof(shape_t), SHAPES, f);
            fclose(f);
#endif
            return;
        }
#else
        if (teststep % 100 == 0)
        {
            printf("Step:\t\t Improvement:%d Total:%d (%5.2f%%)\n",
                    beststep, teststep,(((float)beststep)/((float)teststep))*100);
            printf("Resemblance:\t %0.6f%%\n", ((MAX_FITNESS-lowestdiff) / (float)MAX_FITNESS)*100);
        }
#endif

        if (teststep % 100 == 0 && XPending(dpy))
        {
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
    }
}

void parse_sizearg(int* poly, int* point, char* arg)
{
    char seperator = 'x';
    *poly = *point = 0;
    char* ptr = arg;
    while(*ptr != seperator)
    {
        if (*ptr >= '0' && *ptr <= '9')
            *poly = (*poly * 10) + (*ptr) - 48;
        ptr++;
    }
    ptr++; /* increment past seperator */
    while(*ptr != '\0')
    {
        if (*ptr >= '0' && *ptr <= '9')
            *point = (*point * 10) + (*ptr) - 48;
        ptr++;
    }
}

void dump_prompt()
{
    char prompt[256];
    printf("\nDo you wish to dump your dna? (y/n)");
    scanf("%255s", prompt);
    printf(prompt);
    if (strcmp(prompt, "y") == 0 || strcmp(prompt, "yes") == 0)
    {
        char filename[256];
        printf("Filename: ");
        scanf("%255s", filename);
        save_dna(dna_best, filename);
        exit(EXIT_SUCCESS);    
    }
    else if (strcmp(prompt, "n") == 0 || strcmp(prompt, "no") == 0)
    {
        exit(EXIT_SUCCESS); 
    }
    else
    {
        printf("Invalid entry.");
        dump_prompt();
    }

}

void usage()
{
    printf("Usage:\n\tmona [OPTIONS] [PNG FILE]\n");
    printf("Options\n");
    printf("\t-s [ShapeNum]x[PointNum]\t Changes the number of shapes and points\n");
    printf("\t-l [DnaFile]\tLoads a Dna JSON file\n");
}

int main(int argc, char ** argv) {
    signal(SIGINT, dump_prompt);
    /* parse command line arguments */
    int c;
    int flag_loaddna = 0;
    char* dna_filename;
    while ( (c = getopt(argc, argv, "s:l:")) != -1)
    {
        switch(c)
        {
        case 's':
            parse_sizearg(&SHAPES, &POINTS, optarg);
            break;
        case 'l':
            flag_loaddna = 1;
            dna_filename = optarg;
            break;
        default:
            printf("unknown option %c", (char)c);
            break;
        }
    }
    /* load image from png*/
    cairo_surface_t * pngsurf;
    if (argv[optind] == NULL)
    	usage();
    else
    {
        pngsurf = cairo_image_surface_create_from_png(argv[optind]);

        WIDTH = cairo_image_surface_get_width(pngsurf);
        HEIGHT = cairo_image_surface_get_height(pngsurf);

        /* start main loop  */
        srandom(getpid() + time(NULL));
        x_init();
        if (flag_loaddna)
            dna_best = load_dna(dna_filename);
        else
            dna_best = init_dna();
        mainloop(dna_best, pngsurf);
    }
}

