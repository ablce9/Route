#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "./buffer.h"

#define str5comp(str, c0, c1, c2, c3, c4) \
    *(uint32_t *) str == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) \
	&& (str[4] == c4)

int main() {
    const char input_buffer[] = {'i', 'n', 'p', 'u', 't', '_', 'b', 'u', 'f', '\0'};

    __buffer_t *buf, *dest_buf;
    buf = alloc_new_buffer(input_buffer);
    assert(buf->size == strlen(input_buffer) + 1);
    assert(buf->bytes != input_buffer);

    dest_buf = buffer_bytes_ncpy(buf, 5);
    assert(dest_buf->size == 5);
    assert(str5comp(dest_buf->bytes, 'i', 'n', 'p', 'u', 't') == 1);

    free(buf->bytes);
    free(buf);
    free(dest_buf->bytes);
    free(dest_buf);

    printf("[info] test_buffer.c finished as expected!\n");
    exit(0);
}
