#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "./buffer.h"

__buffer_t *alloc_new_buffer(const char *buffer) {
    __buffer_t *buf = calloc(1, sizeof(__buffer_t));
    int buffer_size = 0;
    char ch;

    while ((ch = buffer[buffer_size])) {
	++buffer_size;
    }
    ++buffer_size;
    buf->bytes = malloc(sizeof(char) * (buffer_size));

    buf->start = buf->bytes;
    buf->pos = buf->start;
    buf->end = buf->bytes + buffer_size;
    buf->size = buffer_size;

    memcpy(buf->bytes, buffer, buffer_size);

    return buf;
}
