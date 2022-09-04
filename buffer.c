#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffer.h"
#include "region.h"

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
    if (!new_region) {
        return NULL;
    }

    buf = (__buffer_t *)new_region->data;

    buf->start = malloc(size);
    if (!buf->start) {
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

__buffer_t *split_chain_buffer(__buffer_t *src_buf, size_t size) {
    size_t remained_space_size;

    remained_space_size = (src_buf->end - src_buf->pos);

    // Add for line terminator.
    size += 1;

    printf("r=%ld\n", remained_space_size);
    if (remained_space_size <= size) {
	printf("[debug] No space left for buffer, allocating new space: %ld bytes\n", size);
	region_t *r = create_chain_buffer(src_buf->r, size + src_buf->size);
	src_buf = r->data;
	src_buf->r = r;
    }

    src_buf->pos[size] = '\0';
    src_buf->pos += size;

    return src_buf;
}
