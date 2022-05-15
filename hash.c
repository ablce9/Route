#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "region.h"

static void cleanup_buckets(void *p) {
    void             *tmp;
    rex_hash_entry_t **buckets;

    tmp = p;
    tmp += sizeof(region_t);

    buckets = ((rex_hash_table_t *)tmp)->buckets;

    free(buckets);
}

static void cleanup_bucket_size(void *p) {
    void             *tmp;
    rex_hash_entry_t *bucket;

    tmp = p;
    tmp += sizeof(region_t);

    bucket = ((rex_hash_entry_t *)tmp);
    free(bucket->current_bucket_size);
}

region_t *init_hash_table(region_t *r) {
    size_t           table_size;
    region_t         *new_region;
    rex_hash_table_t *table;

    new_region = ralloc(r, sizeof(rex_hash_table_t));
    if (new_region == NULL) {
	return NULL;
    }

    table = new_region->data;

    table->r = new_region;

    table_size = 255;

    table->buckets = calloc(sizeof(rex_hash_entry_t *), table_size);

    if (table->buckets == NULL) {
	return NULL;
    }

    new_region->cleanup = cleanup_buckets;
    new_region->data = table;

    return new_region;
}

#ifndef MAX_BUCKETS_SIZE
#define MAX_BUCKETS_SIZE 255
#endif

#ifndef INITIAL_BUCKET_SIZE
#define INITIAL_BUCKET_SIZE 255
#endif
rex_hash_table_t *hash_insert(rex_hash_table_t *table, char *key, char *value, uint8_t key_size) {
    void             *m;
    size_t           buckets_size, current_max_bucket_size, *current_bucket_size;
    uint8_t          hash;
    region_t         *new_region;
    rex_hash_entry_t *bucket, *head, *entry;

    hash = compute_hash_key(key, key_size);
    buckets_size = MAX_BUCKETS_SIZE;
    hash = hash % buckets_size;

    bucket = table->buckets[hash];

    if (bucket) {
	head = bucket;
	current_bucket_size = bucket->current_bucket_size;
	current_max_bucket_size = bucket->max_bucket_size;

	*bucket->current_bucket_size += 1;
	if (bucket->max_bucket_size < *bucket->current_bucket_size) {
	    m = realloc((void *)bucket, sizeof(rex_hash_entry_t) * 255);
	    bucket = (rex_hash_entry_t *)m;
	}

	entry = ++bucket;

	entry->current_bucket_size = current_bucket_size;
	entry->key = key;
	entry->value = value;
	entry->next = NULL;
	entry->prev = head;
	entry->max_bucket_size = current_max_bucket_size;
    } else {
	new_region = ralloc(table->r, sizeof(rex_hash_entry_t) * (size_t)INITIAL_BUCKET_SIZE);
	new_region->cleanup = cleanup_bucket_size;

	table->buckets[hash] = new_region->data;

	table->buckets[hash]->current_bucket_size = malloc(sizeof(size_t));
	*table->buckets[hash]->current_bucket_size = 0;
	table->buckets[hash]->max_bucket_size = (size_t)INITIAL_BUCKET_SIZE;

	entry = table->buckets[hash];

	entry->key = key;
	entry->value = value;

	table->buckets[hash]->next = NULL;
	table->buckets[hash]->prev = NULL;
	table->r = new_region;
    }

    table->buckets[hash] = entry;

    return table;
}

rex_hash_entry_t *find_hash_entry(rex_hash_table_t *table, char *key, size_t key_size) {
    uint8_t          hash, entry_size;
    rex_hash_entry_t *bucket, *p;

    hash = compute_hash_key(key, key_size);
    entry_size = 255;
    hash = hash % entry_size;

    bucket = table->buckets[hash];

    if (bucket == NULL) {
	return NULL;
    }

    p = bucket;

    while ((p != NULL)) {
	if (strncmp(p->key, key, key_size) == 0) {
	    return p;
	}

	p = p->prev;
    }

    return NULL;
}

size_t compute_hash_key(const char *data, const uint8_t size) {
    size_t key, i;

    key = 0;

#define hash_key(key, c) ((uint8_t)key * 31 + c);
    for (i = 0; i < size; i++) {
	key = hash_key(key, data[i]);
    }
#undef hash_key

    return key;
}
