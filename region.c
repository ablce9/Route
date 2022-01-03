#include <stdlib.h>
#include "region.h"

#define POOL_ALIGNMENT 8
#define ALIGN_PADDING(x) ((__SIZE_MAX__ - (x) + 1) & (POOL_ALIGNMENT - 1))

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

void destroy_regions(region_t *r) {
    region_t *p, *tmp, *current;

    current = r->current;

    p = current->next;
    while (p) {
        tmp = p;
        p = p->next;
        free(tmp);
    };

    free(current);
}

region_t *ralloc(region_t *r, size_t region_size) {
    region_t *new;
    void *m;

    size_t base_size = sizeof(region_size) + region_size;
    m = malloc(base_size + ALIGN_PADDING(base_size));

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
