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

    if (bucket) {
	free(bucket->current_bucket_size);
    }
}

region_t *init_hash_table(region_t *r) {
    region_t         *table_r;
    rex_hash_table_t *table;

    table_r = ralloc(r, sizeof(rex_hash_table_t));
    if (table_r == NULL) {
	return NULL;
    }

    table = table_r->data;

    table->r = table_r;

    table->buckets = calloc(sizeof(rex_hash_entry_t *), INITIAL_BUCKET_SIZE);

    if (table->buckets == NULL) {
	return NULL;
    }

    table_r->cleanup = cleanup_buckets;
    table_r->data = table;

    return table_r;
}

rex_hash_table_t *hash_insert(rex_hash_table_t *table, char *key, char *value, uint8_t key_size) {
    size_t           current_max_bucket_size, *current_bucket_size;
    uint8_t          hash;
    region_t         *bucket_r;
    rex_hash_entry_t *bucket, *head, *entry;

    hash = compute_hash_key(key, key_size);
    hash = hash % INITIAL_BUCKET_SIZE;

    bucket = table->buckets[hash];
    if (bucket != NULL) {
	head = bucket;
	current_bucket_size = bucket->current_bucket_size;
	current_max_bucket_size = bucket->max_bucket_size;

	*bucket->current_bucket_size += 1;

	if (bucket->max_bucket_size < *bucket->current_bucket_size) {
	    printf("[debug] Reallocating space for bucket entries\n");
	    bucket->r = reallocate_region(bucket->r, bucket->r->size + BUCKET_ENTRY_SIZE);
	    table->r = bucket->r;

	    bucket->max_bucket_size += BUCKET_ENTRY_COUNT;
	}

	entry = bucket + *bucket->current_bucket_size;
	entry->current_bucket_size = current_bucket_size;
	entry->key = key;
	entry->value = value;
	entry->next = NULL;
	entry->prev = head;
	entry->max_bucket_size = current_max_bucket_size;
	entry->r = bucket->r;

    } else {
	bucket_r = ralloc(table->r, sizeof(rex_hash_entry_t) * INITIAL_BUCKET_SIZE);
	if (bucket_r == NULL) {
	    return NULL;
	}

	bucket_r->cleanup = cleanup_bucket_size;

	table->buckets[hash] = bucket_r->data;
	table->buckets[hash]->current_bucket_size = malloc(sizeof(size_t));
	if (table->buckets[hash]->current_bucket_size == NULL) {
	    return NULL;
	}

	*table->buckets[hash]->current_bucket_size = 1;
	table->buckets[hash]->max_bucket_size = (size_t)INITIAL_BUCKET_SIZE;

	entry = table->buckets[hash];

	entry->key = key;
	entry->value = value;
	entry->r = bucket_r;

	table->buckets[hash]->r = bucket_r;
	table->buckets[hash]->next = NULL;
	table->buckets[hash]->prev = NULL;
	table->r = bucket_r;
    }

    table->buckets[hash] = entry;

    return table;
}

rex_hash_entry_t *find_hash_entry(rex_hash_table_t *table, char *key, size_t key_size) {
    uint8_t          hash;
    rex_hash_entry_t *bucket, *entry;

    hash = compute_hash_key(key, key_size);
    hash = hash % INITIAL_BUCKET_SIZE;

    bucket = table->buckets[hash];

    if (bucket == NULL) {
	return NULL;
    }

    entry = bucket;

    while ((entry != NULL)) {
	if (strncmp(entry->key, key, key_size) == 0) {
	    return entry;
	}

	entry = entry->prev;
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
