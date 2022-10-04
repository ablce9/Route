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

void test_alloc_string() {
    { // when strs is out of space
	rex_string_cylinder_t *cylinder;
	rex_string_t *str1, *str2, *str3;
	region_t *r;

	size_t alloced_strs_num = 1;

	r = init_region();

	r = ralloc(r, sizeof(rex_string_cylinder_t));
	cylinder = r->data;
	cylinder->r = r;

	init_string_cylinder(cylinder, alloced_strs_num, 128);

	str1 = alloc_string(cylinder, "test", 4)->strs;
	// strs head stays still
	assert(cylinder->strs == str1);
	// str_space gets alloc-ed
	assert(cylinder->str_space_end - cylinder->str_space == 124);

	str2 = alloc_string(cylinder, "foo", 3)->strs;
	// strs is alloc-ed again
	assert(str1 != str2);
	// str_space gets alloc-ed
	assert(cylinder->str_space_end - cylinder->str_space == 121);

	str3 = alloc_string(cylinder, "bar", 3)->strs;
	// strs is alloc-ed again
	assert(str2 != str3);
	// str_space gets alloc-ed
	assert(cylinder->str_space_end - cylinder->str_space == 118);

	destroy_regions(cylinder->r);
    }

    { // when str_space is out of space
	rex_string_cylinder_t *cylinder;
	rex_string_t *str1, *str2, *str3;
	region_t *r;

	size_t alloced_str_space_byte = 128;

	r = init_region();

	r = ralloc(r, sizeof(rex_string_cylinder_t));
	cylinder = r->data;
	cylinder->r = r;

	init_string_cylinder(cylinder, 10, alloced_str_space_byte);

	char buf[128];
	str1 = alloc_string(cylinder, generate_entropy(buf, 128), 128)->strs;
	// strs head stays still
	assert(cylinder->strs == str1);
	// str_space gets alloc-ed
	assert(cylinder->str_space_end - cylinder->str_space == 0);

	str2 = alloc_string(cylinder, "foo", 3)->strs;
	// strs is alloc-ed again
	assert(str1 != str2);
	// str_space gets alloc-ed
	assert(cylinder->str_space_end - cylinder->str_space == 125);

	str3 = alloc_string(cylinder, "bar", 3)->strs;
	// strs is alloc-ed again
	assert(str2 != str3);
	// str_space gets alloc-ed
	assert(cylinder->str_space_end - cylinder->str_space == 122);

	destroy_regions(cylinder->r);
    }
}

int main() {
    test_alloc_string();
}
