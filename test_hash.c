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
    __hash_entry_t *entry;

    r = create_region();
    r = init_hash_table(r);

    table = (rex_hash_table_t *)r->data;

    table = hash_insert(table, "host", "google.com", 4);
    r = table->r;

    table = hash_insert(table, "referer", "https://google.com/", 4);
    r = table->r;

    entry = find_hash_entry(table, "host", 4);
    printf("value=%s\n", entry->value);

    entry = find_hash_entry(table, "referer", 4);
    printf("value=%s\n", entry->value);

    entry = find_hash_entry(table, "foo", 3);
    printf("value=%p\n", entry);

    destroy_regions(r);

    return 0;
};
