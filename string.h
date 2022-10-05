#ifndef __STRING_H
#define __STRING_H

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

rex_string_cylinder_t *init_string_cylinder(rex_string_cylinder_t *cylinder, size_t string_count, size_t space_size);
rex_string_cylinder_t *alloc_string(rex_string_cylinder_t *cylinder, char *src_str, size_t len);
#endif // __STRING_H
