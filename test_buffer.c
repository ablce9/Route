#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "buffer.h"
#include "region.h"

static char *generate_entropy(char *buf, int max) {
    int i;

    char *words = "abcdefghrjklmnopqrstuvwxyzABCDEFGHRJKLMNOPQRSTUVWXYZ0123456789";
    for (i = 0; i < max; i++) {
	buf[i] = words[rand() % 62];
    };
    buf[max] = '\0';

    return buf;
}

int test_buffer() {
    __buffer_t *buf;
    region_t *r;
    size_t size = 50;

    r = create_region();
    buf = create_chain_buffer(r, size);

    int max;
    time_t t;
    srand((unsigned) time(&t));

    max = rand() % 4049;
    char out1[max], *p;

    generate_entropy(out1, max);

    p = split_chain_buffer(buf, sizeof(out1));
    memcpy(p, out1, sizeof(out1));

    max = rand() % 4049;
    char out2[max];

    generate_entropy(out2, max);

    p = split_chain_buffer(buf, sizeof(out2));
    memcpy(p, out2, sizeof(out2));

    destroy_regions(r);
    return 0;
}

int main() {
    test_buffer();
}
