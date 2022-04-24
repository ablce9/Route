#include <stdint.h>
#include <stdlib.h>

#include "hash.h"
#include "region.h"

region_t *init_hash(region_t *r, size_t size) {
    __hash_t  *hash;
    region_t *new_region;

    new_region = ralloc(r, sizeof(__hash_t) * size);
    if (new_region == NULL) {
	return NULL;
    }

    hash = (__hash_t *)new_region->data;
    hash->r = new_region;

    return (region_t *)hash;
}

__hash_t *make_hash(__hash_t *hashs) {
    __hash_t *hash;

    hash = hashs;
    hash->key = NULL;
    hash->value = NULL;

    hashs += sizeof(__hash_t);

    return hash;
}
