#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffer.h"
#include "region.h"

static __buffer_t *realloc_chain_buffer(__buffer_t *buf, size_t want) {
    void   *new_space;
    size_t new_space_size;

    new_space_size = want * sizeof(char *);

    new_space = realloc((void *)buf->start, new_space_size);

    buf->start = (char *)new_space;
    buf->pos   = buf->start;
    buf->end   = buf->start + new_space_size;

    buf->size += new_space_size;

    return buf;
}

static void cleanup(void *p) {
    void       *tmp;
    __buffer_t *buf;

    tmp = p;

    tmp += sizeof(region_t);
    buf = (__buffer_t *)tmp;
    free(buf->start);
}

region_t *create_chain_buffer(region_t *r, size_t size) {
    region_t   *new_region;
    __buffer_t *buf;

    new_region = ralloc(r, sizeof(__buffer_t));
    if (new_region == NULL) {
	return NULL;
    }

    buf = (__buffer_t *)new_region->data;

    size = size * sizeof(char *);
    buf->start = malloc(size);
    if (buf->start == NULL) {
	return NULL;
    }

    memset(buf->start, 0, size);

    buf->r          = new_region;
    buf->r->cleanup = cleanup;
    buf->pos        = buf->start;
    buf->end        = buf->start + size;
    buf->size       = size;

    new_region->data = buf;

    return new_region;
}

char *split_chain_buffer(__buffer_t *src_buf, size_t size) {
    char *new;
    long remained_space_size;

    remained_space_size = (src_buf->end - src_buf->pos);

    if (remained_space_size < (long)(size * sizeof(char *))) {
	src_buf = realloc_chain_buffer(src_buf, size);
	printf("[debug] No space left for buffer, allocating new space: %ld bytes\n", size * sizeof(char *));
    }

    new = src_buf->pos;
    src_buf->pos += size * sizeof(char *);

    return new;
}
