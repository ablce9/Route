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

    char *chars = "abcdefghrjklmnopqrstuvwxyzABCDEFGHRJKLMNOPQRSTUVWXYZ0123456789";
    for (i = 0; i < max; i++) {
	buf[i] = chars[rand() % 62];
    };
    buf[max] = '\0';

    return buf;
}

// int test_buffer() {
//     rex_string_buffer_t *buf;
//     region_t *r;
//     size_t size = 50;
//
//     r = create_region();
//     r = create_chain_buffer(r, size);
//     buf = (rex_string_buffer_t *)r->data;
//
//     int max;
//     time_t t;
//     srand((unsigned) time(&t));
//
//     max = rand() % 4049;
//     char out1[max], *p;
//
//     generate_entropy(out1, max);
//
//     buf = alloc_string_buffer(buf, sizeof(out1));
//     p = buf->pos;
//     memcpy(p, out1, sizeof(out1));
//
//     max = rand() % 4049;
//     char out2[max];
//
//     generate_entropy(out2, max);
//
//     buf = alloc_string_buffer(buf, sizeof(out2));
//     p = buf->pos;
//     memcpy(p, out2, sizeof(out2));
//
//     destroy_regions(r);
//     return 0;
// }

void test_alloc_string_buffer() {
    char *d1, *d2, *p1, *p2;
    region_t *r;
    rex_string_buffer_t *buf;

    r = create_region();
    r = create_chain_buffer(r, 10);
    buf = r->data;

    d1 = "12345";
    p1 = buf->pos;
    buf = alloc_string_buffer(buf, p1, d1, 5);

    d2 = "54321";
    p2 = buf->pos;
    buf = alloc_string_buffer(buf, p2, d2, 5);

    // assert(p1 == d1);
    // assert(p2 == d2);

    printf("p1=%s\n", p1);
    printf("p2=%s\n", p2);
    destroy_regions(buf->r);
}

void test_string() {
    rex_string_cylinder_t *cylinder;
    rex_string_t *str1, *str2, *str3, *str4, *str5;
    region_t *r;

    r = create_region();

    r = ralloc(r, sizeof(rex_string_cylinder_t));
    cylinder = r->data;
    cylinder->r = r;

    init_string_cylinder(cylinder, 4, 100);
    str1 = alloc_string(cylinder, "test", 4)->strs;
    str2 = alloc_string(cylinder, "foo", 3)->strs;
    str3 = alloc_string(cylinder, "bar", 3)->strs;
    str4 = alloc_string(cylinder, "abc", 3)->strs;
    str5 = alloc_string(cylinder, "rab", 3)->strs;

    printf("str=%s, len=%ld\n", str1->string, str1->len);
    printf("str=%s, len=%ld\n", str2->string, str2->len);
    printf("str=%s, len=%ld\n", str3->string, str3->len);
    printf("str=%s, len=%ld\n", str4->string, str4->len);
    printf("str=%s, len=%ld\n", str5->string, str5->len);

    destroy_regions(cylinder->r);
}

int main() {
    //test_buffer();
    //test_alloc_string_buffer();
    test_string();
}
