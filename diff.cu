/* from dhowey, howey on github */
#include <cuda.h>
#include <cairo.h>
#include <math.h>
#include "mona.h"

extern "C" {
	int difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf);
	int get_max_fitness(void);
}
//The CUDA block size
#define BLOCK_SIZE 16

int MAX_FITNESS = -1;

unsigned char * goal_data = NULL;
unsigned char * goal_data_d;

__global__ void differenceKernel(unsigned char * test_data, unsigned char * goal_data, int * difference, int * my_max_fitness, int width, int height)
{
    int tx = threadIdx.x + blockIdx.x * blockDim.x;
    int ty = threadIdx.y + blockIdx.y * blockDim.y;
    int i = tx * width + ty;

    int difference_s = 0;
    int my_max_fitness_s = 0;

    if(i < height*width) {
	    int thispixel = 4 * i;
	    //int thispixel = tx*WIDTH*4 + ty*4;

	    unsigned char test_a = test_data[thispixel];
	    unsigned char test_r = test_data[thispixel + 1];
	    unsigned char test_g = test_data[thispixel + 2];
	    unsigned char test_b = test_data[thispixel + 3];

            unsigned char goal_a = goal_data[thispixel];
	    unsigned char goal_r = goal_data[thispixel + 1];
	    unsigned char goal_g = goal_data[thispixel + 2];
	    unsigned char goal_b = goal_data[thispixel + 3];

	    my_max_fitness_s += goal_a + goal_r + goal_g + goal_b;

	    difference_s += (ABS(test_a - goal_a) + ABS(test_r - goal_r) + ABS(test_g - goal_g) + ABS(test_b - goal_b));
    }

    my_max_fitness[i] = my_max_fitness_s;
    difference[i] = difference_s;
}

int difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf)
{
    unsigned char * test_data = cairo_image_surface_get_data(test_surf);
    if(!goal_data) {
        goal_data = cairo_image_surface_get_data(goal_surf);
    	cudaMemcpy(goal_data_d, goal_data, sizeof(unsigned char)*4*WIDTH*HEIGHT, cudaMemcpyHostToDevice);
    }

    unsigned char * test_data_d;
    int * difference;
    int * my_max_fitness;
    int * difference_d;
    int * my_max_fitness_d;
    dim3 blockDim(BLOCK_SIZE, BLOCK_SIZE, 1);
    dim3 gridDim(ceil((float)WIDTH/(float)BLOCK_SIZE), ceil((float)HEIGHT/(float)BLOCK_SIZE), 1);

    //TODO: Make these pointers global and only malloc once during the entire program
    cudaMalloc((void **)&test_data_d, sizeof(unsigned char)*4*WIDTH*HEIGHT);
    cudaMalloc((void **)&difference_d, sizeof(int)*WIDTH*HEIGHT);
    cudaMalloc((void **)&my_max_fitness_d, sizeof(int)*WIDTH*HEIGHT);
    difference = (int *)malloc(sizeof(int)*WIDTH*HEIGHT);
    my_max_fitness = (int *)malloc(sizeof(int)*WIDTH*HEIGHT);

    //This will really slow things down. PCI-E bus will be a bottleneck.
    cudaMemcpy(test_data_d, test_data, sizeof(unsigned char)*4*WIDTH*HEIGHT, cudaMemcpyHostToDevice);

    //Launch the kernel to compute the difference
    differenceKernel<<<gridDim, blockDim>>>(test_data_d, goal_data_d, difference_d, my_max_fitness_d, WIDTH, HEIGHT);

    //Copy results from the device, another PCI-E bottleneck
    cudaMemcpy(difference, difference_d, sizeof(int)*WIDTH*HEIGHT, cudaMemcpyDeviceToHost);
    cudaMemcpy(my_max_fitness, my_max_fitness_d, sizeof(int)*WIDTH*HEIGHT, cudaMemcpyDeviceToHost);

    /*
    int difference = 0;

    int my_max_fitness = 0;

    #pragma omp parallel for 
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

            if(MAX_FITNESS == -1)
                my_max_fitness += goal_a + goal_r + goal_g + goal_b;

		#pragma omp atomic
	    difference += (ABS(test_a - goal_a) + ABS(test_r - goal_r) + ABS(test_g - goal_g) + ABS(test_b - goal_b));
        }
    }
    */

    //TODO: perform reduction on the GPU. Probalby won't be much speedup anyways
    int my_max_fitness_total = 0;
    int difference_total = 0;

    for(int i = 0; i < WIDTH*HEIGHT; i++) {
	    my_max_fitness_total += my_max_fitness[i];
	    difference_total += difference[i];
    }

    if(MAX_FITNESS == -1)
        MAX_FITNESS = my_max_fitness_total;

    cudaFree(test_data_d);
    cudaFree(difference_d);
    cudaFree(my_max_fitness_d);
    free(difference);
    free(my_max_fitness);

    return difference_total;
}

int get_max_fitness()
{
	return MAX_FITNESS;
}
