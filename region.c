#include <stdlib.h>
#include <string.h>
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

	if (tmp->cleanup != NULL) {
	    tmp->cleanup(tmp);
	}

	free(tmp);
    };

    free(current);
}

region_t *ralloc(region_t *r, size_t region_size) {
    size_t   base_size;
    region_t *new;

    base_size = sizeof(region_t) + region_size;
    new = malloc(base_size);
    if (new == NULL) {
	return NULL;
    }

    memset(new, 0, base_size);

    new->next = NULL;
    new->current = r->current;
    new->cleanup = NULL;

    r->next = new;

    return new;
}

void* alloc_from_region(region_t *region, size_t size) {
    void *p;

    p = region + size;

    return p;
}
