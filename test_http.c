#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "./http.h"
#include "./buffer.h"

#define CRLF "\r\n"

static int test_parse_http_request_header() {
    http_request_header_t *request_header = calloc(1, sizeof(http_request_header_t));
    char *raw_header = "GET / HTTP/1.1" CRLF \
	"Host: google.com" CRLF \
	"User-Agent: curl/7.74.0" CRLF \
	"Accept: */*" CRLF;
    __buffer_t *header_buf = alloc_new_buffer(raw_header);
    route_int result = parse_http_request_header(request_header, header_buf);

    assert(result == ROUTE_OK);
    assert(request_header->method == HTTP_REQUEST_METHOD_GET);
    assert(strncmp(request_header->meta[0]->key->bytes, (char*)"Host", 4) == 0);
    assert(strncmp(request_header->meta[1]->key->bytes, (char*)"User-Agent: curl/7.74.0", 10) == 0);
    assert(strncmp(request_header->meta[2]->key->bytes, (char*)"Accept", 6) == 0);
    printf("[info] test_http.c finished as expected!\n");
    exit(0);
}

int main() {
    exit(test_parse_http_request_header());
}
