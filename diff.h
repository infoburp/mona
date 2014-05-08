#include <cairo.h>

int difference_init();
void difference_clean();
int difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf);
int get_max_fitness(void);
