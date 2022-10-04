#ifndef __REGION_H
#define __REGION_H

#include <unistd.h>

#define ALIGNMENT sizeof(unsigned long)
#define align_ptr(p, a)							\
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

typedef void (*cleanup_t)(void *p);
typedef struct region_s region_t;
struct region_s {
    size_t     size;
    void       *data;
    region_t   *current;
    region_t   *next;
    cleanup_t  cleanup;
};

region_t *init_region();
region_t *ralloc(region_t *r, size_t region_size);
void *ralloc2(region_t *r, size_t region_size);
void destroy_regions(region_t *r);
void* alloc_from_region(region_t *region, size_t size);
region_t *reallocate_region(region_t *r, size_t region_size);

#endif // __REGION_H
