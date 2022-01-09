#ifndef __BUFFER_H
#define __BUFFER_H

#include <stdio.h>

#include "region.h"

typedef struct {
    size_t size;
    char *bytes;
    char *pos;
    char *start;
    char *end;
} __buffer_t;

__buffer_t *create_chain_buffer(region_t *r, size_t size);
__buffer_t *alloc_new_buffer(const char *buffer);
__buffer_t *buffer_bytes_ncpy(const __buffer_t *src, const size_t size);

#endif // __BUFFER_H
