#include <stdbool.h>
#include <stdlib.h>

#define RANDINT(max) (int)((rand() / (double)RAND_MAX) * (max))
#define RANDDOUBLE(max) ((rand() / (double)RAND_MAX) * max)
#define ABS(val) ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))

extern int WIDTH;
extern int HEIGHT;
