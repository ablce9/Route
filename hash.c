#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "region.h"

static void cleanup(void *p) {
    void           *tmp;
    __hash_entry_t **buckets;

    tmp = p;
    tmp += sizeof(region_t);

    buckets = ((rex_hash_table_t *)tmp)->buckets;

    free(buckets);
}

region_t *init_hash_table(region_t *r) {
    size_t         table_size;
    region_t       *new_region;
    rex_hash_table_t *table;

    new_region = ralloc(r, sizeof(rex_hash_table_t));
    if (new_region == NULL) {
	return NULL;
    }

    table = new_region->data;

    table->r = new_region;

    table_size = 255;

    table->buckets = calloc(sizeof(__hash_entry_t), table_size);

    if (table->buckets == NULL) {
	return NULL;
    }

    new_region->cleanup = cleanup;
    new_region->data = table;

    return new_region;
}

rex_hash_table_t *hash_insert(rex_hash_table_t *table, char *key, char *value, uint8_t key_size) {
    uint8_t           hash, entry_size;
    region_t          *new_region;
    __hash_entry_t    *bucket, *head, *entry;

    hash = compute_hash_key(key, key_size);
    entry_size = 255;
    hash = hash % entry_size;

    bucket = table->buckets[hash];

    if (bucket == NULL) {
	new_region = ralloc(table->r, sizeof(__hash_entry_t) * entry_size);
	table->buckets[hash] = new_region->data;

	entry = table->buckets[hash];

	entry->key = key;
	entry->value = value;

	table->buckets[hash]->next = NULL;
	table->buckets[hash]->prev = NULL;

	table->r = new_region;

    } else {
	head = bucket;
	entry = ++bucket;

	entry->key = key;
	entry->value = value;
	entry->next = NULL;
	entry->prev = head;
    }

    table->buckets[hash] = entry;

    return table;
}

__hash_entry_t *find_hash_entry(rex_hash_table_t *table, char *key, size_t key_size) {
    uint8_t        hash, entry_size;
    __hash_entry_t *bucket, *p;

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


size_t compute_hash_key(char *data, uint8_t size) {
    size_t key, i;

    key = 0;

#define hash_key(key, c) ((uint8_t)key * 31 + c);
    for (i = 0; i < size; i++) {
	key = hash_key(key, data[i]);
    }
#undef hash_key

    return key;
}
