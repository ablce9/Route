#include <stdlib.h>
#include "region.h"

region_t *create_region() {
    region_t *r;
    size_t max_regions_size;

    r = malloc(sizeof(region_t));
    if (r == NULL) {
	return NULL;
    }

    max_regions_size = 16 * 1024;
    r->max = max_regions_size;

    r->current = r;
    r->next = NULL;

    return r;
}

void destroy_region(region_t *r) {
    free(r);
}

region_t *ralloc(region_t *r, size_t region_size) {
    region_t *new;
    void *m;

    m = malloc(sizeof(region_t) + region_size);
    if (m == NULL) {
	return NULL;
    }

    new = (region_t*)m;
    new->next = NULL;
    new->current = r->current;

    r->next = new;

    return new;
}

void* alloc_from_region(region_t *region, size_t size) {
    void *p;

    p = region + size;

    return p;
}
