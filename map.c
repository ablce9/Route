#include <stdlib.h>

#include "map.h"

__map_t *new_map(void) {
    __map_t *map = calloc(1, sizeof(__map_t));

    map->key = NULL;
    map->value = NULL;

    return map;
}
