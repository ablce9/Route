#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "buffer.h"
#include "region.h"

#define str5comp(str, c0, c1, c2, c3, c4) \
    (*(uint32_t *) str == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) && (str[4] == c4))

int test_buffer() {
    const char input_buffer[] = {'i', 'n', 'p', 'u', 't', '\0'};

    __buffer_t *buf, *dest_buf;
    buf = alloc_new_buffer(input_buffer);
    assert(buf->size == strlen(input_buffer) + 1);
    assert(str5comp(buf->bytes, 'i', 'n', 'p', 'u', 't') == 1);
    assert(str5comp(buf->bytes, 'k', 'n', 'p', 'u', 't') == 0);

    dest_buf = buffer_bytes_ncpy(buf, 5);
    assert(dest_buf->size == 5);
    assert(str5comp(dest_buf->bytes, 'i', 'n', 'p', 'u', 't') == 1);
    assert(str5comp(dest_buf->bytes, 'i', 'm', 'p', 'u', 't') == 0);

    free(buf->bytes);
    free(buf);
    free(dest_buf->bytes);
    free(dest_buf);

    printf("[info] test_buffer.c finished as expected!\n");

    return 0;
}

int test_create_chain_buffer() {
    __buffer_t *buf;
    region_t *r;
    char *a, *b;

    r = create_region();
    buf = create_chain_buffer(r, sizeof(char *) * 10);

    a = buf->pos;
    a = "a";
    buf->pos += 1;

    b = buf->pos;
    b = "b";
    buf->pos += 2;

    printf("%s, %s\n", a, b);

    destroy_regions(r);
    free(buf);
    return 0;
}

int main() {
    // test_buffer();
    exit(test_create_chain_buffer());
}
