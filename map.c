#include <stdlib.h>

#include "map.h"
#include "region.h"

region_t *init_map(region_t *r, size_t size) {
    void     *m;
    __map_t  *map;
    region_t *new;

    m = ralloc(r, sizeof(__map_t));
    if (m == NULL) {
	return NULL;
    }

    new = (region_t *)m;
    m += sizeof(region_t);

    map = (__map_t *)new;
    map->r = new;

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
