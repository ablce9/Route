#include <stdio.h>

typedef struct {
    size_t size;
    char *bytes;
    char *pos;
    char *start;
    char *end;
} __buffer_t;

__buffer_t *alloc_new_buffer(const char *buffer);
