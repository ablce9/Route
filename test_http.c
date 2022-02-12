#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "./http.h"
#include "./buffer.h"
#include "region.h"

#define CRLF "\r\n"

static int test_parse_http_request() {
    http_header_t *request_header = calloc(1, sizeof(http_header_t));
    char *raw_header = "GET / HTTP/1.1" CRLF \
	"Host: google.com" CRLF \
	"User-Agent: curl/7.74.0" CRLF \
	"Accept: */*" CRLF;
    __buffer_t reqb, *chain_buf;
    __map_t *map;
    region_t *r;

    r = create_region();
    r = (region_t *)create_chain_buffer(r, sizeof(char *) * 2048 * 16);

    chain_buf = (__buffer_t *)r;

    r = init_map(r, 1024);
    map = (__map_t *)r;

    reqb.pos = raw_header;
    reqb.end = reqb.pos + sizeof(raw_header) * sizeof(char *);

    route_int result = parse_http_request(request_header, &reqb, chain_buf, map);

    assert(result == ROUTE_OK);
    assert(request_header->method == HTTP_REQUEST_METHOD_GET);
    // assert(strncmp(request_header->meta[0]->key->bytes, (char*)"Host", 4) == 0);
    // assert(strncmp(request_header->meta[1]->key->bytes, (char*)"User-Agent: curl/7.74.0", 10) == 0);
    // assert(strncmp(request_header->meta[2]->key->byte s, (char*)"Accept", 6) == 0);
    printf("[info] test_http.c finished as expected!\n");

    return 0;
}

static int test_make_http_header() {
    __buffer_t *buf;
    region_t *r;
    char *header1, *header2, *header3;
    http_header_t h;

    r = create_region();

    buf = create_chain_buffer(r, sizeof(char *) * 5000 * 3);

    h.size = 1;
    header1 = make_http_response_header(&h, buf);

    h.size = 2;
    header2 = make_http_response_header(&h, buf);

    h.size = 3;
    header3 = make_http_response_header(&h, buf);

    printf("%s%s%s", header1, header2, header3);

    destroy_regions(r);
    return 0;
}

int main() {
    /* todo: free memory */
    test_parse_http_request();
    test_make_http_header();

    time_t t;

    t = time(&t);
    char http_time[1024];
    memset(http_time, 0, sizeof(http_time));
    make_http_time(&t, http_time);
    printf("%s\n", http_time);
    return 0;
}
