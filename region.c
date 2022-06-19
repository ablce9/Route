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
    region_t *new;

    assert(r->size < new_region_size);

    new = ralloc(r, new_region_size);
    if (new == NULL) {
	return NULL;
    }

    // TODO: investigate why valgrind complains if we do memcpy
    // ==637== Invalid read of size 8
    // ==637==    at 0x483F7F7: memmove (vg_replace_strmem.c:1270)
    // ==637==    by 0x109B57: reallocate_region (region.c:92)
    // ==637==    by 0x109603: hash_insert (hash.c:83)
    // ==637==    by 0x109C38: main (test_hash.c:23)
    // ==637==  Address 0x4a186c8 is 0 bytes after a block of size 14,376 alloc'd
    // ==637==    at 0x483877F: malloc (vg_replace_malloc.c:307)
    // ==637==    by 0x109A54: ralloc (region.c:61)
    // ==637==    by 0x109B23: reallocate_region (region.c:86)
    // ==637==    by 0x109603: hash_insert (hash.c:83)
    // ==637==    by 0x109C19: main (test_hash.c:22)
    // memcpy(new->data, r->data, r->size);
    new->data = r->data;
    new->data += r->size;
    return new;
}

void* alloc_from_region(region_t *region, size_t size) {
    void *p;

    p = region + size;

    return p;
}
