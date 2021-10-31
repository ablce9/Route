#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "./http_request.h"
#include "./buffer.h"
#include "./map.h"


http_request_header_t *new_http_request_header(void) {
    http_request_header_t *header = calloc(1, sizeof(http_request_header_t));

    header->start = NULL;
    header->method = 0;
    header->start_length = 0;

    return header;
}

http_response_payload_t *new_http_response_payload(char *header, char *body) {
    http_response_payload_t *http_payload = malloc(sizeof(http_response_payload_t));
    http_response_payload_t hrp = {
	.header = header,
	.body = body
    };
    *http_payload = hrp;
    return http_payload;
}

http_request_payload_t *new_http_request_payload(void) {
    http_request_header_t *header = new_http_request_header();
    http_request_payload_t *request = calloc(1, sizeof(http_request_payload_t));

    http_request_payload_t req = {
	.header = header
    };
    *request = req;

    return request;
}

route_int
parse_http_request_header_start(http_request_header_t *header, const char *line_buf) {
    __buffer_t *buf = alloc_new_buffer(line_buf);

    header->start = buf;

    enum {
	start = 0,
	method,
	http_path
    } state;

    state = start;

    char *p, ch, *request_start;
    for (p = buf->pos; p < buf->end; p++) {

	ch = *p;

	switch (state) {

	case start:
	    request_start = p;
	    state = method;
	    break;

	case method:
	    if (ch == ' ') {

		switch(p - buf->start) {

		case 3:
		    if (str4comp(request_start, 'G', 'E', 'T', ' ')) {
			header->method = HTTP_REQUEST_METHOD_GET;
			state = http_path;
			break;
		    }

		    if (str4comp(request_start, 'P', 'U', 'T', ' ')) {
			header->method = HTTP_REQUEST_METHOD_PUT;
			state = http_path;
			break;
		    }

		    goto error;

		case 4:
		    if (str4comp(request_start, 'P', 'O', 'S', 'T')) {
			header->method = HTTP_REQUEST_METHOD_POST;
			state = http_path;
			break;
		    }

		    if (str4comp(request_start, 'H', 'E', 'A', 'D')) {
			header->method = HTTP_REQUEST_METHOD_POST;
			state = http_path;
			break;
		    }

		    goto error;

		case 5:
		    if (str5comp(request_start, 'P', 'A', 'T', 'C', 'H')) {
			header->method = HTTP_REQUEST_METHOD_PATCH;
			state = http_path;
			break;
		    }

		default:
		    goto error;

		}
	    }

	case http_path:
	    break;

	default:
	    goto error;

	}
    }

    if (header->method == 0) {
	goto error;
    }

    return ROUTE_OK;

 error:
    return ROUTE_ERROR;
}


#define MAX_HTTP_HEADER_LINE_BUFFER_SIZE 4096

#define LF '\n'
#define CR '\r'

route_int
parse_http_request_header(http_request_header_t *header, const __buffer_t *header_buf) {
    enum {
	start = 0,
	almost_done,
	done
    } state;

    char line[MAX_HTTP_HEADER_LINE_BUFFER_SIZE], ch;
    int i = 0, line_length = 0, map_index = 0;
    __map_t *map;

    state = start;

    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE);

    route_int parsed_status;

    // Parse http method and version
    for (i = 0; i < header_buf->size; ++i) {

	ch = header_buf->bytes[i];

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
	    if (state == almost_done && line_length <= MAX_HTTP_HEADER_LINE_BUFFER_SIZE) {
		line[line_length] = '\0';
		parsed_status = parse_http_request_header_start(header, line);

		if (parsed_status != ROUTE_OK) {
		    goto error;
		}

		state = done;
	    }
	    break;

	default:
	    line[line_length] = ch;
	    ++line_length;

	}

	if (state == done) {
	    break;
	}
    }

    if (!header->start) {
	goto error;
    }

    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE); // reset
    state = start;
    line_length = 0;
    int start_length = header->start->size + 1;

    // Parse http meta headers
    for (; start_length < header_buf->size; ++start_length) {

	ch = header_buf->bytes[start_length];

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
	    /* TODO: skip last http header CRLF line */
	    if (state == almost_done && line_length <= MAX_HTTP_HEADER_LINE_BUFFER_SIZE) {
		map = parse_http_request_meta_header_line(line);

		if (map != NULL) {
		    header->meta[map_index] = map;
		    ++map_index;
		}
	    };

	    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE); // reset
	    line_length = 0;
	    state = done;
	    break;

	default:
	    line[line_length] = ch;
	    ++line_length;

	}
    }

    return ROUTE_OK;

 error:
    return ROUTE_ERROR;
}

__map_t *parse_http_request_meta_header_line(const char *line_buf) {
    __buffer_t *buf = alloc_new_buffer(line_buf);
    if (buf == NULL) {
	return NULL;
    }

    __map_t *map = new_map();
    char ch, *p;

    for (p = buf->pos; p < buf->end; p++) {

	ch = *p;

	if (ch == ':') {
	    map->key = buffer_bytes_ncpy(buf, p - buf->start);
	    break;
	}
    }

    ++p;
    buf->pos += p - buf->start;

    for (p = buf->pos; p < buf->end; p++) {

	ch = *p;

	if (ch == '\0') {
	    __buffer_t *value_buf = alloc_new_buffer(buf->bytes + map->key->size + 1 /* colon */ + 1 /* whitespace */);

	    if (value_buf != NULL) {
		map->value = (void*)value_buf;
	    }
	}
    }

    free(buf->bytes);
    free(buf);

    return map;
}
