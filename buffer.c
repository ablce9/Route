#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "buffer.h"
#include "region.h"

static rex_string_cylinder_t *append_string(rex_string_cylinder_t *cylinder, char *src_str, size_t len) {
    rex_string_t *str;
    ssize_t remained_strs_size;

    remained_strs_size = cylinder->strs_end - cylinder->strs;
    if (cylinder->strs_count >= 1) {
	cylinder->strs += 1;
	remained_strs_size -= 1;
    }

    if (remained_strs_size <= 0) {
	// error
	return NULL;
    }

    str = cylinder->strs;
    str->len = len;
    str->string = cylinder->str_space;
    str->string = src_str;
    str->end = str->string + len;

    cylinder->strs = str;
    cylinder->str_space += len;
    cylinder->strs_count += 1;

    return cylinder;
}

#define LEAST_STR_SPACE_BYTES 127
rex_string_cylinder_t *init_string_cylinder(rex_string_cylinder_t *cylinder, size_t strings_count, size_t space_size) {
    region_t *r;
    rex_string_t *strs;

    if (space_size <= LEAST_STR_SPACE_BYTES) {
	printf("error: str_space must be larger than 127\n");
	return NULL;
    }

    r = ralloc(cylinder->r, sizeof(rex_string_t) * strings_count);
    memset(r->data, 0, sizeof(rex_string_t) * strings_count);
    strs = r->data;

    r = ralloc(r, sizeof(char *) * space_size);
    cylinder->r = r;
    cylinder->str_space = r->data;

    if (!cylinder->str_space) {
	return NULL;
    }

    cylinder->str_space_size = space_size;
    cylinder->str_space_end = cylinder->str_space + space_size;
    cylinder->strs = strs;
    cylinder->strs_max_size = strings_count;
    cylinder->strs_end = strs + strings_count;
    cylinder->strs_count = 0;

    return cylinder;
}

static rex_string_cylinder_t *refresh_string_cylinder(rex_string_cylinder_t *cylinder, size_t strings_count) {
    region_t *r;
    rex_string_t *strs;

    r = ralloc(cylinder->r, sizeof(rex_string_t) * strings_count);
    memset(r->data, 0, sizeof(rex_string_t) * strings_count);
    strs = r->data;

    cylinder->r = r;
    cylinder->strs = strs;
    cylinder->strs_max_size = strings_count;
    cylinder->strs_end = strs + strings_count;
    cylinder->strs_count = 0;

    return cylinder;
}

rex_string_cylinder_t *alloc_string(rex_string_cylinder_t *cylinder, char *src_str, size_t len) {
    ssize_t remained_strs_size, remained_space;

    remained_strs_size = cylinder->strs_end - cylinder->strs;

    if (remained_strs_size <= 1) {
	refresh_string_cylinder(cylinder, cylinder->strs_max_size);
    }

    remained_space = cylinder->str_space_end - cylinder->str_space;

    if (remained_space <= 0) {
	region_t *r = ralloc(cylinder->r, (sizeof(char *)* cylinder->str_space_size) + labs(remained_space));
	cylinder->str_space = r->data;

	if (!cylinder->str_space) {
	    return NULL;
	}
	cylinder->str_space_end = cylinder->str_space + cylinder->str_space_size;
	cylinder->r = r;
    }

    append_string(cylinder, src_str, len);

    return cylinder;
}
