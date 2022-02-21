#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "./http.h"
#include "./buffer.h"
#include "./map.h"

#define CRLF "\r\n"

static void format_string(char *string, size_t size, char *fmt, ...);

const char *weeks[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static __map_t *parse_http_request_meta_header_line(char *line_buf, __map_t *map, __buffer_t *chain_buf) {
    char ch, *p, *key_buf, *val_buf;
    size_t key_size = 0;

    for (p = line_buf; ; key_size++, p++) {

	ch = *p;

	if (ch == ':') {
	    char key[key_size];

	    memcpy(key, line_buf, key_size);
	    key[key_size] = '\0';

	    BUFFER_MOVE(key_buf, chain_buf->pos, key_size);
	    memcpy(key_buf, key, key_size);
	    map->key = key_buf;

	    break;
	}
    }

    // TODO: add line length as param
    BUFFER_MOVE(val_buf, chain_buf->pos, strnlen(line_buf, 1024) - key_size);
    val_buf = line_buf + key_size + 2;
    map->value = (void *)val_buf;

    return map;
}

static void format_string(char *string, size_t size, char *fmt, ...)
{
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   vsnprintf(string, size, fmt, arg_ptr);
   va_end(arg_ptr);
}

static char *make_http_time(time_t *t, char *buf) {
    struct tm *p;

    p = gmtime(t);

    sprintf(buf, "%s, %02d %s %d %02d:%02d:%02d GMT",
	    weeks[p->tm_wday],
	    p->tm_mday,
	    months[p->tm_mon],
	    1900 + p->tm_year,
	    p->tm_hour,
	    p->tm_min,
	    p->tm_sec);

    return buf;
}

char *make_http_response_header(http_header_t *h, char *chain_buf) {
    char *fmt =								\
	"HTTP/1.1 200 OK\r\n"						\
	"Server: route\r\n" \
	"Content-Length: %zu\r\n" \
	"Date: %s\r\n"				\
	"Content-Type: text/html; charset=UTF-8\r\n"			\
	CRLF;
    char buf[4096], http_time_buf[64];
    size_t buf_size = sizeof(buf);
    time_t now;

    now = time(&now);

    make_http_time(&now, http_time_buf);

    format_string(chain_buf, buf_size, fmt, h->size, http_time_buf);

    return chain_buf;
}

http_header_t *new_http_request_header(region_t *r) {
    region_t      *new_region;
    http_header_t *header;

    new_region = ralloc(r, sizeof(http_header_t));
    if (new_region == NULL) {
	return NULL;
    }

    header = (http_header_t *)new_region->data;

    header->r = new_region;
    header->start = NULL;
    header->method = 0;
    header->start_size = 0;

    return header;
}

http_request_payload_t *new_http_request_payload(region_t *r) {
    region_t               *new_region;
    http_header_t          *header;
    http_request_payload_t *request;

    new_region = ralloc(r, sizeof(http_request_payload_t));
    if (new_region == NULL) {
	return NULL;
    }

    request = (http_request_payload_t *)new_region->data;
    if (request == NULL) {
	return NULL;
    }

    header = new_http_request_header(new_region);
    if (header == NULL) {
	return NULL;
    }

    request->header = header;
    request->r = header->r;

    return request;
}

route_int
parse_http_request_start(http_header_t *header, const char *line_buf) {
    enum {
	start = 0,
	method,
	http_path
    } state;

    state = start;

    char *p, ch, *request_start;
    for (p = (char *)line_buf; p < line_buf + sizeof(line_buf); p++) {

	ch = *p;

	switch (state) {

	case start:
	    request_start = p;
	    state = method;
	    break;

	case method:
	    if (ch == ' ') {

		switch(p - line_buf) {

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
parse_http_request(http_header_t *header, __buffer_t *reqb, __buffer_t *chain_buf, __map_t *maps) {
    enum {
	start = 0,
	almost_done,
	done
    } state;

    char line[MAX_HTTP_HEADER_LINE_BUFFER_SIZE], ch, *p;
    int line_length = 0, map_index = 0;
    route_int parsed_status;
    __map_t *map;

    state = start;
    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE);

    for (p = reqb->pos; p < reqb->end; p++) {

	ch = *p;

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
	    if (state == almost_done && line_length <= MAX_HTTP_HEADER_LINE_BUFFER_SIZE) {

		line[line_length] = '\0';

		parsed_status = parse_http_request_start(header, line);

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

    BUFFER_MOVE(header->start, chain_buf->pos, line_length);

    header->start_size = line_length;

    if (!header->start) {
	goto error;
    }

    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE); // reset
    state = start;
    line_length = 0;

    __buffer_t header_rest;
    header_rest.pos = reqb->pos + header->start_size;
    header_rest.end = reqb->pos + (sizeof(char *) * header->start_size);

    for (p = header_rest.pos; p < header_rest.end; ++p) {

	ch = *p;

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
	    if (state == almost_done && line_length <= MAX_HTTP_HEADER_LINE_BUFFER_SIZE && line_length > 0) {

		char *line_buf;

		BUFFER_MOVE(line_buf, chain_buf->pos, line_length);
		memcpy(line_buf, line, line_length);
		line_buf[line_length] = '\0';

		map = make_map(maps);
		map = parse_http_request_meta_header_line(line_buf, map, chain_buf);

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
