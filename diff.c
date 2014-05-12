#include <stdlib.h>
#include <cairo.h>
#include "mona.h"

int MAX_FITNESS = -1;

unsigned char *goal_data = NULL;

int difference_init()
{
	return 0;
}

void difference_clean()
{
}

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

                        // On little-endian machine test_data and goal_data have BGRA bytes order.
                        // Since pre-multiplied alpha is used, we don`t need alpha byte at all.
                        // FIXME: will not work on big-endian, need to fix.
			unsigned char test_b = test_data[thispixel];
			unsigned char test_g = test_data[thispixel + 1];
			unsigned char test_r = test_data[thispixel + 2];

			unsigned char goal_b = goal_data[thispixel];
			unsigned char goal_g = goal_data[thispixel + 1];
			unsigned char goal_r = goal_data[thispixel + 2];

			if (MAX_FITNESS == -1)
				my_max_fitness += goal_r + goal_g + goal_b;

			difference += ABS(test_r - goal_r);
			difference += ABS(test_g - goal_g);
			difference += ABS(test_b - goal_b);
		}
	}

	if (MAX_FITNESS == -1)
		MAX_FITNESS = my_max_fitness;
	return difference;
}

int get_max_fitness(void)
{
	return MAX_FITNESS;
}
