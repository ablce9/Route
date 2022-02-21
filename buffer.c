#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffer.h"
#include "region.h"

static void cleanup(void *p) {
    __buffer_t *buf;

    p += sizeof(region_t);
    buf = (__buffer_t *)p;
    free(buf->start);
}

__buffer_t *create_chain_buffer(region_t *r, size_t size) {
    region_t   *new_region;
    __buffer_t *buf;

    new_region = ralloc(r, sizeof(__buffer_t));
    if (new_region == NULL) {
	return NULL;
    }

    buf = (__buffer_t *)new_region->data;

    buf->start = malloc(size);
    if (buf->start == NULL) {
	return NULL;
    }

    memset(buf->start, 0, size);

    buf->r = new_region;
    buf->r->cleanup = cleanup;
    buf->pos = buf->start;
    buf->end = buf->start + size;
    buf->size = size;

    return buf;
}
