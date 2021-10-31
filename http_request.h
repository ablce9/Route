#ifndef __HTTP_REQUEST_H
#define __HTTP_REQUEST_H

#include <stdint.h>
#include <string.h>

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
    __buffer_t  *start;
    uint8_t      method;
    int          start_length;
    __map_t    *meta[1024];
} http_request_header_t;

typedef struct {
    char *header;
    char *body;
} http_response_payload_t;

typedef struct {
    http_request_header_t *header;
    // todo: add body
} http_request_payload_t;

route_int parse_http_request_header_start(http_request_header_t *header, const char *line_buf);
route_int parse_http_request_header(http_request_header_t *header, const __buffer_t *header_buf);
__map_t *parse_http_request_meta_header_line(const char *line_buf);
http_request_header_t *new_http_request_header(void);
http_response_payload_t* new_http_response_payload(char *header, char *body);
http_request_payload_t *new_http_request_payload(void);

#endif //  __HTTP_REQUEST_H
