#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "region.h"

int main() {
    region_t *r;
    int i;
    void *ptr;

    r = create_region();
    for (i = 1; i < 10; i++) {
	r = ralloc(r, 513 * sizeof(char *));
	ptr = r + sizeof(region_t);
	ptr = "foo";
    }

    printf("c: %s\n", (char *)ptr);

    destroy_regions(r);
}
