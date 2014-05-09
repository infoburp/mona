#ifndef MONA_H
#define MONA_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    double x, y;
} point_t;

typedef struct {
    double r, g, b, a;
    point_t* points;
} shape_t;

extern int WIDTH;
extern int HEIGHT;

extern int SHAPES;
extern int POINTS;

#endif
