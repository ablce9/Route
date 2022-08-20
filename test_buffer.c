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
    r = create_chain_buffer(r, size);
    buf = (__buffer_t *)r->data;

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

void test_split_chain_buffer() {
    char *d1, *d2, *p1, *p2;
    region_t *r;

    r = create_region();
    r = create_chain_buffer(r, 12);

    d1 = "12345";
    p1 = split_chain_buffer(r->data, 5);
    p1 = d1;

    d2 = "54321";
    p2 = split_chain_buffer(r->data, 5);
    p2 = d2;

    printf("p1=%s\n", p1);
    printf("p2=%s\n", p2);
    destroy_regions(r);
}

int main() {
    //test_buffer();
    test_split_chain_buffer();
}
