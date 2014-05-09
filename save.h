#ifndef SAVE_H
#define SAVE_H

#include "mona.h"
#include "cJSON.h"

cJSON* load_cJSON_node_from_file(const char *);
void save_cJSON_node_to_file(cJSON*, const char *);

/* generates a cJSON node from a shape and vice versa */
cJSON* write_shape_node(shape_t);
shape_t read_shape_node(cJSON*);
/* saves dna to file */
void save_dna(shape_t* dna, const char* filename);
/* loads dna from file */
shape_t* load_dna(const char* filename);
#endif
