#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "hash.h"
#include "region.h"

int main() {
    region_t       *r;
    rex_hash_table_t *table;
    rex_hash_entry_t *entry;

    r = create_region();
    r = init_hash_table(r);

    table = (rex_hash_table_t *)r->data;

    hash_insert(table, "host", "google.com", 5);
    hash_insert(table, "host", "fuga", 5);
    hash_insert(table, "host", "okay", 5);
    hash_insert(table, "referer", "https://google.com/", 8);

    entry = find_hash_entry(table, "host", 5);
    printf("value=%s\n", entry->value);

    entry = find_hash_entry(table, "referer", 8);
    printf("value=%s\n", entry->value);

    entry = find_hash_entry(table, "foo", 4);
    printf("value=%p\n", entry);

    destroy_regions(table->r);

    return 0;
};
