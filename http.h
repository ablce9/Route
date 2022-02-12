#ifndef __HTTP_H
#define __HTTP_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "./buffer.h"
#include "./map.h"

#define HTTP_REQUEST_METHOD_GET  0x1
#define HTTP_REQUEST_METHOD_PUT  0x2
#define HTTP_REQUEST_METHOD_POST 0x4
#define HTTP_REQUEST_METHOD_HEAD 0x5
#define HTTP_REQUEST_METHOD_PATCH 0x6

#define MAX_HTTP_HEADER_LINE_BUFFER_SIZE 4096

#define ROUTE_OK 0
#define ROUTE_ERROR -1

#define str4comp(str, c0, c1, c2, c3) *(uint32_t *) str == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define str5comp(str, c0, c1, c2, c3, c4) \
    (*(uint32_t *) str == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) && (str[4] == c4))

typedef intptr_t route_int;

typedef struct {
    char *start;
    uint8_t      method;
    size_t       size;
    int          start_length;
    __map_t    *meta[1024];
} http_header_t;

typedef struct {
    char *header;
    char *body;
} http_response_payload_t;

typedef struct {
    http_header_t *header;
    // todo: add body
} http_request_payload_t;

route_int parse_http_request_header_start(http_header_t *header, const char *line_buf);
route_int parse_http_request_header(http_header_t *header, __buffer_t *header_buf);
__map_t *parse_http_request_meta_header_line(const char *line_buf);
region_t *new_http_request_header(region_t *r);
region_t *new_http_request_payload(region_t *r);
char *make_http_response_header(http_header_t *h, __buffer_t *chain_buf);
char *make_http_time(time_t *t, char *buf);

#endif //  __HTTP_H
