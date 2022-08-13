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
    http_header_t *header;
    char *raw_header = "GET / HTTP/1.1" CRLF \
	"Host: google.com" CRLF \
	"User-Agent: curl/7.74.0" CRLF \
	"Accept: */*" CRLF;
    __buffer_t reqb, *chain_buf;
    region_t *r;

    r = create_region();
    r = (region_t *)create_chain_buffer(r, sizeof(char *) * strlen(raw_header));

    chain_buf = (__buffer_t *)r;

    reqb.pos = raw_header;
    reqb.end = reqb.pos + strlen(raw_header);

    r = init_http_header(r);
    header = (http_header_t *)r->data;

    rex_int result = parse_http_request(header, &reqb, chain_buf);

    assert(result == REX_OK);
    assert(header->method == HTTP_REQUEST_METHOD_GET);
    assert(strcmp(fetch_header_field("Host", 4, header)->value, "google.com") == 0);
    assert(strcmp(fetch_header_field("User-Agent", 10, header)->value, "curl/7.74.0") == 0);
    assert(strcmp(fetch_header_field("Accept", 6, header)->value, "*/*") == 0);
    printf("[info] test_http.c finished as expected!\n");

    return 0;
}

static int test_make_http_header() {
    char          *buf, *header1, *header2, *header3;
    region_t      *r;
    http_header_t h;

    r = create_region();

    r = create_chain_buffer(r, 5000);
    buf = ((__buffer_t *)r->data)->pos;

    h.size = 1;
    header1 = create_http_response_header(&h, buf);

    h.size = 2;
    header2 = create_http_response_header(&h, buf);

    h.size = 3;
    header3 = create_http_response_header(&h, buf);

    printf("%s%s%s", header1, header2, header3);

    destroy_regions(r);
    return 0;
}

static int test_create_http_request_header_string() {
    char       *buf;
    region_t   *r;

    r = create_region();

    r = create_chain_buffer(r, 5000);
    buf = ((__buffer_t *)r->data)->pos;

    http_header_t h = {
	.path = "/index.html",
	.size = 0,
	.method = HTTP_REQUEST_METHOD_GET,
	.version = 0
    };

    buf = create_http_request_header_string(&h, buf);

    printf("%s", buf);

    destroy_regions(r);

    return 0;
}

int main() {
    /* todo: free memory */
    test_parse_http_request();
    // test_make_http_header();
    //
    // time_t t;
    //
    // t = time(&t);
    // char http_time[1024];
    // memset(http_time, 0, sizeof(http_time));
    //
    // printf("%s", http_time);
    //
    // test_create_http_request_header_string();

    return 0;
}
