#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

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
    r->size = max_regions_size;
    r->current = r;
    r->next = NULL;
    r->data = NULL;

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
    new_region->next = NULL;
    new_region->current = r->current;
    new_region->cleanup = NULL;
    new_region->data = m;
    new_region->size = region_size;

    r->next = new_region;

    return new_region;
}

region_t *reallocate_region(region_t *r, size_t new_region_size) {
    size_t original_size;
    region_t *new;
    void *original_data;

    original_size = r->size;
    original_data = r->data;

    new = ralloc(r, new_region_size + original_size);
    if (new == NULL) {
	return NULL;
    }

    memcpy(new->data, original_data, original_size);

    return new;
}

void* alloc_from_region(region_t *region, size_t size) {
    void *p;

    p = region + size;

    return p;
}
