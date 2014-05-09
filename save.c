#include "save.h"

void save_cJSON_node_to_file(cJSON* save, const char* filename)
{
    FILE* file = fopen(filename, "w");
    fprintf(file, cJSON_Print(save));
    fclose(file);
}

cJSON* load_cJSON_node_from_file(const char* filename)
{
    FILE* file = fopen(filename, "r");
    char* buffer;
    if (file)
    {
        long length;
        fseek (file, 0, SEEK_END);
        length = ftell (file);
        fseek (file, 0, SEEK_SET);
        buffer = malloc (sizeof(char) * length);
        fread (buffer, 1, length, file);
        fclose(file);
        return cJSON_Parse(buffer);
    }
    else
    {
        return NULL;
    }
}

cJSON* write_shape_node(shape_t shape)
{
    cJSON* save = cJSON_CreateObject();
    cJSON_AddItemToObject(save, "r", cJSON_CreateNumber(shape.r));
    cJSON_AddItemToObject(save, "g", cJSON_CreateNumber(shape.g));
    cJSON_AddItemToObject(save, "b", cJSON_CreateNumber(shape.b));
    cJSON_AddItemToObject(save, "a", cJSON_CreateNumber(shape.a));
    cJSON* coord_array = cJSON_CreateArray();
    for (int i = 0; i < POINTS; i++)
    {
        cJSON* obj = cJSON_CreateObject();
        cJSON_AddItemToObject(obj, "x", cJSON_CreateNumber(shape.points[i].x));
        cJSON_AddItemToObject(obj, "y", cJSON_CreateNumber(shape.points[i].y));
        cJSON_AddItemToArray(coord_array, obj);
    }
    cJSON_AddItemToObject(save, "coords", coord_array);
    return save;
}

shape_t read_shape_node(cJSON* node)
{
    cJSON* coord_array = cJSON_GetObjectItem(node, "coords");
    //if (!coord_array || coord_array->type != cJSON_Array)
    //    return NULL;
    assert(cJSON_GetArraySize(coord_array) == POINTS);
    cJSON *r, *g, *b, *a;
    r = cJSON_GetObjectItem(node, "r");
    g = cJSON_GetObjectItem(node, "g");
    b = cJSON_GetObjectItem(node, "b");
    a = cJSON_GetObjectItem(node, "a");
    //if (!r || !g || !b || !a)
    //    return NULL;
    shape_t shape;
    shape.points = malloc(sizeof(point_t) * POINTS);
    for (int i = 0; i < POINTS; i++)
    {
        cJSON* obj = cJSON_GetArrayItem(coord_array, i);
        struct cJSON *x, *y;
        x = cJSON_GetObjectItem(obj, "x");
        y = cJSON_GetObjectItem(obj, "y");
        shape.points[i].x = x->valuedouble;
        shape.points[i].y = y->valuedouble;
    }
    shape.r = (*r).valuedouble;
    shape.g = (*g).valuedouble;
    shape.b = (*b).valuedouble;
    shape.a = (*a).valuedouble;
    return shape;
}

void save_dna(shape_t* dna, const char* filename)
{
    cJSON* save = cJSON_CreateObject();
    cJSON_AddItemToObject(save, "shapes", cJSON_CreateNumber(SHAPES));
    cJSON_AddItemToObject(save, "points", cJSON_CreateNumber(POINTS));
    cJSON* shape_array = cJSON_CreateArray();
    for (int i = 0; i < SHAPES; i++)
        cJSON_AddItemToArray(shape_array, write_shape_node(dna[i]));
    cJSON_AddItemToObject(save, "shapes_array", shape_array); 
    save_cJSON_node_to_file(save, filename);
}

shape_t* load_dna(const char* filename)
{
    cJSON* node = load_cJSON_node_from_file(filename);
    if (!node)
        return NULL;
    cJSON* shape_array = cJSON_GetObjectItem(node, "shapes_array");
    cJSON* shapes = cJSON_GetObjectItem(node, "shapes");
    cJSON* points = cJSON_GetObjectItem(node, "points");
    if (!shape_array || shape_array->type != cJSON_Array)
        return NULL;
    if (!shapes || !points)
        return NULL;
    SHAPES = shapes->valueint;
    POINTS = points->valueint;
    assert(cJSON_GetArraySize(shape_array) == SHAPES);
    shape_t* dna = malloc(sizeof(shape_t) * SHAPES);
    for (int i = 0; i < SHAPES; i++)
        dna[i] = read_shape_node(cJSON_GetArrayItem(shape_array, i));
    return dna;
}
