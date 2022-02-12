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

char *make_http_time(time_t *t, char *buf) {
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

char *make_http_response_header(http_header_t *h, __buffer_t *chain_buf) {
    char *fmt =								\
	"HTTP/1.1 200 OK\r\n"						\
	"Server: route\r\n" \
	"Content-Length: %zu\r\n" \
	"Date: %s\r\n"				\
	"Content-Type: text/html; charset=UTF-8\r\n"			\
	CRLF;
    char buf[4049], http_time_buf[64], *header;
    size_t buf_size = sizeof(buf);
    time_t now;

    memset(http_time_buf, 0, sizeof(http_time_buf));
    now = time(&now);
    make_http_time(&now, http_time_buf);

    memset(buf, 0, buf_size);
    format_string(buf, buf_size, fmt, h->size, http_time_buf);

    memcpy(chain_buf->pos, buf, buf_size);
    BUFFER_MOVE(header, chain_buf->pos, buf_size);

    return header;
}

region_t *new_http_request_header(region_t *r) {
    http_header_t *header;

    r = ralloc(r, sizeof(http_header_t));
    header = (http_header_t *)r;
    header->start = NULL;
    header->method = 0;
    header->start_length = 0;

    return r;
}

region_t *new_http_request_payload(region_t *r) {
    region_t *new = new_http_request_header(r);
    http_header_t *header = (http_header_t *)new;
    http_request_payload_t *request;

    new = ralloc(new, sizeof(http_request_payload_t));
    request =  (http_request_payload_t *)new;
    request->header = header;

    return new;
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
parse_http_request(http_header_t *header, __buffer_t *reqb, __buffer_t *chain_buf) {
    enum {
	start = 0,
	almost_done,
	done
    } state;

    char line[MAX_HTTP_HEADER_LINE_BUFFER_SIZE], ch, *p;
    int line_length = 0, map_index = 0;
    __map_t *map;
    route_int parsed_status;

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
    memcpy(header->start, line, line_length);

    if (!header->start) {
	goto error;
    }

    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE); // reset
    state = start;
    line_length = 0;

    for (p = header->start; p < header->start + sizeof(header->start); ++p) {

	ch = *p;

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
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

static void format_string(char *string, size_t size, char *fmt, ...)
{
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   vsnprintf(string, size, fmt, arg_ptr);
   va_end(arg_ptr);
}
