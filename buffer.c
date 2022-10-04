#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "buffer.h"
#include "region.h"

static void cleanup(void *p) {
    void       *tmp;
    rex_string_buffer_t *buf;

    tmp = p;

    tmp += sizeof(region_t);
    buf = (rex_string_buffer_t *)tmp;
    free(buf->start);
}

static void cleanup_string_cylinder_space(void *p) {
    void                  *tmp;
    rex_string_cylinder_t *cylinder;

    tmp = p;

    tmp += sizeof(region_t);
    cylinder = (rex_string_cylinder_t *)tmp;

    if (cylinder->str_space) {
	free(cylinder->str_space); // _end - cylinder->str_space_size);
	cylinder->str_space = NULL;
    }
}

region_t *create_chain_buffer(region_t *r, size_t size) {
    region_t   *new_region;
    rex_string_buffer_t *buf;

    new_region = ralloc(r, sizeof(rex_string_buffer_t));
    if (!new_region) {
	return NULL;
    }

    buf = (rex_string_buffer_t *)new_region->data;

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

rex_string_buffer_t *alloc_string_buffer(rex_string_buffer_t *buf, char *dst, char *src, size_t size) {
    size_t remained_space_size;

    remained_space_size = buf->end - buf->pos;

    // Add for line terminator.
    size += 1;

    if (remained_space_size <= size) {
	printf("[debug] No space left for buffer, allocating new space: %ld bytes\n", size);
	region_t *r = create_chain_buffer(buf->r, size);
	buf = r->data;
	memcpy(dst, src, size);
	buf->r = r;
    } else {
	memcpy(dst, src, size);
    }

    buf->pos[size] = '\0';
    buf->pos += size;

    return buf;
}

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

    // TODO: Use calloc over malloc. Avoid memset.
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

static rex_string_cylinder_t *reinit_string_cylinder(rex_string_cylinder_t *cylinder, size_t strings_count, size_t space_size) {
    region_t *r;
    rex_string_t *strs;

    // TODO: Use calloc over malloc. Avoid memset.
    r = ralloc(cylinder->r, sizeof(rex_string_t) * strings_count);
    memset(r->data, 0, sizeof(rex_string_t) * strings_count);
    strs = r->data;

    cylinder->r = r;
    //cylinder->r->cleanup = cleanup_string_cylinder_space;
    //cylinder->str_space = malloc(sizeof(char *) * space_size);
    //if (!cylinder->str_space) {
    //	return NULL;
    //}

    //cylinder->str_space_size = space_size;
    //cylinder->str_space_end = cylinder->str_space + space_size;
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
	reinit_string_cylinder(cylinder, cylinder->strs_max_size, cylinder->str_space_size);
    }

    remained_space = cylinder->str_space_end - cylinder->str_space;
    if (remained_space <= 0) {
	region_t *r = ralloc(cylinder->r, (sizeof(char *)* cylinder->str_space_size) + labs(remained_space));
	r->cleanup = cleanup_string_cylinder_space;
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
