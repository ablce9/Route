#ifndef __MAP_H
#define __MAP_H

#include "./buffer.h"
#include "./region.h"

typedef struct {
    char *key;
    void *value;
} __map_t;

region_t *init_map(region_t *r, size_t size);
__map_t *make_map(__map_t *map);
#endif // __MAP_H
