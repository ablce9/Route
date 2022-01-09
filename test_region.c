#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "region.h"

typedef struct {
    int x;
    int y;
} point_t ;

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
	    point = (point_t*)(r + sizeof(region_t) + sizeof(point_t) * ii);

	    point->x = rand();
	    point->y = rand();

	    printf("x=%d, y=%d\n", point->x, point->y);
	}
    }

    destroy_regions(r);
}
