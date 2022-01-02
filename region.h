#ifndef __REGION_H
#define __REGION_H

typedef struct region_s region_t;
struct region_s {
    size_t max;

    region_t *current;
    region_t *next;
};

region_t *create_region();
region_t *ralloc(region_t *r, size_t region_size);
void destroy_region(region_t *r);
void* alloc_from_region(region_t *region, size_t size);

#endif // __REGION_H
