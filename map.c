#include <stdlib.h>

#include "map.h"
#include "region.h"

region_t *init_map(region_t *r, size_t size) {
    r = ralloc(r, sizeof(__map_t) + (sizeof(__map_t) * size));

    return r;
}

__map_t *make_map(__map_t *maps) {
    __map_t *map;

    map = maps;
    map->key = NULL;
    map->value = NULL;

    maps += sizeof(__map_t);

    return map;
}
