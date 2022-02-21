#include <stdlib.h>

#include "map.h"
#include "region.h"

region_t *init_map(region_t *r, size_t size) {
    __map_t  *map;
    region_t *new_region;

    new_region = ralloc(r, sizeof(__map_t) * size);
    if (new_region == NULL) {
	return NULL;
    }

    map = (__map_t *)new_region->data;
    map->r = new_region;

    return (region_t *)map;
}

__map_t *make_map(__map_t *maps) {
    __map_t *map;

    map = maps;
    map->key = NULL;
    map->value = NULL;

    maps += sizeof(__map_t);

    return map;
}
