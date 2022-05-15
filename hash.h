#ifndef __HASH_H
#define __HASH_H

#include <stdint.h>

#include "./buffer.h"
#include "./region.h"

typedef struct rex_hash_entry_s rex_hash_entry_t;

struct rex_hash_entry_s {
    char             *key;
    char             *value;
    size_t           *current_bucket_size;
    size_t           max_bucket_size;
    rex_hash_entry_t *next;
    rex_hash_entry_t *prev;
};

typedef struct {
    region_t         *r;
    rex_hash_entry_t **buckets;
} rex_hash_table_t;

region_t *init_entry(region_t *r, size_t size);
region_t *init_hash_table(region_t *r);
rex_hash_table_t *hash_insert(rex_hash_table_t *buckets, char *key, char *value, uint8_t key_size);
rex_hash_entry_t *find_hash_entry(rex_hash_table_t *table, char *key, size_t key_size);
size_t compute_hash_key(const char *data, const uint8_t size);

#endif // __HASH_H
