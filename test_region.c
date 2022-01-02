#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "region.h"

int main() {
    region_t *r, *p;
    int i;

    r = create_region();
    for (i = 0; i < 10; i++) {
	r = ralloc(r, 1024);
    }

    for (p = r->current; p->next; p = p->next) {
	printf("ptr=%p\n", p);
    }
}
