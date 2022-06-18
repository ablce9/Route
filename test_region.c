#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <strings.h>

#include "region.h"
#include "buffer.h"

typedef struct {
    int x;
    int y;
} point_t ;

static void *copy_10_digits_string(void *dst) {
    char payload[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    strncpy(dst, payload, 10);
    return dst;
}

static void *copy_10_reversed_order_digits_string(void *dst) {
    char payload[10] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0'};
    strncpy(dst, payload, 10);
    return dst;
}

void test_reallocate_region() {
    // given region created with char* data
    region_t *r;
    r = create_region();
    r = ralloc(r, 10);
    copy_10_digits_string(r->data);

    // when new space is allocated for region data
    char *old_addr = r->data;
    r = reallocate_region(r, 25, 10);

    char *new_addr = r->data  + 25;

    // then data must be equal
    copy_10_reversed_order_digits_string(new_addr);

    printf("old=%p [%s]\n", old_addr, old_addr);
    assert(strncmp(old_addr, "0123456789", 10) == 0);

    printf("new=%p [%s]\n", new_addr, new_addr);
    assert(strncmp(new_addr, "9876543210", 10) == 0);

    // TODO: fix segfault
    //destroy_regions(r);
}

int main() {
    region_t *r;
    int i, ii;
    time_t t;

    srand((int)time(&t));

    r = create_region();

    for (i = 0; i < 100; i++) {
	r = ralloc(r, 513 * sizeof(point_t));

	for (ii = 0; ii < 10; ii++) {
	    point_t *point;
	    point = (point_t *)(r->data + sizeof(region_t) + (sizeof(point_t) * ii));

	    point->x = rand();
	    point->y = rand();

	    printf("x=%d, y=%d\n", point->x, point->y);
	}
    }

    destroy_regions(r);

    test_reallocate_region();
}
