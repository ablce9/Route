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
    void       *m;
    region_t   *new;
    __buffer_t *buf;

    m = ralloc(r, sizeof(__buffer_t));
    if (m == NULL) {
	return NULL;
    }

    new = (region_t *)m;
    m += sizeof(region_t);
    buf = (__buffer_t *)m;

    buf->start = malloc(size);
    if (buf->start == NULL) {
	return NULL;
    }

    memset(buf->start, 0, size);

    buf->r = new;
    buf->r->cleanup = cleanup;
    buf->pos = buf->start;
    buf->end = buf->start + size;
    buf->size = size;

    return buf;
}
