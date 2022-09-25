#ifndef __BUFFER_H
#define __BUFFER_H

#include <stdio.h>

#include "region.h"
typedef struct rex_string_s {
    char   *end;
    char   *string;
    size_t len;
} rex_string_t;

typedef ssize_t (*compute_strs_size_t)();
typedef struct rex_string_cylinder_s {
    char         *str_space;
    char         *str_space_end;
    size_t        str_space_size;
    region_t     *r;
    rex_string_t *strs;
    size_t        strs_max_size;
    size_t        strs_count;
    rex_string_t *strs_end;
} rex_string_cylinder_t;

typedef struct {
    size_t   size;
    char     *pos;
    char     *start;
    char     *end;
    region_t *r;
} rex_string_buffer_t;

#define BUFFER_CURRENT_SIZE(s, e, p) (s - ((e - p)/sizeof(char *)))

#define BUFFER_MOVE(p, b, s)				\
    do { \
	p = b; \
	b += s * sizeof(char *);		\
    } while(0);

region_t *create_chain_buffer(region_t *r, size_t size);
rex_string_buffer_t *alloc_new_buffer(const char *buffer);
rex_string_buffer_t *buffer_bytes_ncpy(const rex_string_buffer_t *src, const size_t size);
rex_string_buffer_t *alloc_string_buffer(rex_string_buffer_t *buf, char *dst, char *src, size_t size);
rex_string_cylinder_t *init_string_cylinder(rex_string_cylinder_t *cylinder, size_t string_count, size_t space_size);
rex_string_cylinder_t *alloc_string(rex_string_cylinder_t *cylinder, char *src_str, size_t len);
#endif // __BUFFER_H
