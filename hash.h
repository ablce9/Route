#ifndef __HASH_H
#define __HASH_H

#include "./buffer.h"
#include "./region.h"

typedef struct {
    char     *key;
    void     *value;
    region_t *r;
} __hash_t;

typedef struct {

} __hash_key_t;

region_t *init_hash(region_t *r, size_t size);
__hash_t *make_hash(__hash_t *hash);

#endif // __HASH_H
