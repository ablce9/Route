#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "region.h"

region_t *create_region() {
    size_t   max_regions_size;
    region_t *r;

    r = malloc(sizeof(region_t));
    if (r == NULL) {
	return NULL;
    }
    memset(r, 0, sizeof(region_t));

    max_regions_size = 16 * 1024;

    r->cleanup = NULL;
    r->max     = max_regions_size;
    r->current = r;
    r->next    = NULL;
    r->data    = NULL;

    return r;
}

void destroy_regions(region_t *r) {
    region_t *p, *tmp, *current;

    current = r->current;

    p = current->next;

    while (p) {
	tmp = p;
	p = p->next;

	if (tmp->cleanup != NULL) {
	    tmp->cleanup(tmp);
	}

	free(tmp);
    };

    free(current);
}

#define ALIGNMENT sizeof(unsigned long)
#define align_ptr(p, a)							\
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

region_t *ralloc(region_t *r, size_t region_size) {
    void     *m;
    size_t   base_size;
    region_t *new_region;

    base_size = sizeof(region_t) + region_size;
    m = malloc(base_size);
    if (m == NULL) {
	return NULL;
    }

    new_region = (region_t *)m;

    m += sizeof(region_t);
    m = align_ptr(m, ALIGNMENT);

    new_region->next    = NULL;
    new_region->current = r->current;
    new_region->cleanup = NULL;
    new_region->data    = m;

    r->next = new_region;

    return new_region;
}

void* alloc_from_region(region_t *region, size_t size) {
    void *p;

    p = region + size;

    return p;
}
