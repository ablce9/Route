#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <time.h>

#include "./http.h"
#include "./buffer.h"
#include "./hash.h"

#define CRLF "\r\n"

static void format_string(char *string, size_t size, char *fmt, ...);

const char *weeks[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static rex_hash_table_t *parse_http_request_meta_header_line(char *line_buf, size_t line_size, http_header_t *header, __buffer_t *chain_buf) {
    char ch, *p, *key_buf, *val_buf;
    size_t key_size = 0;

    for (p = line_buf; key_size < line_size; key_size++, p++) {

	ch = *p;

	if (ch == ':') {
	    val_buf = split_chain_buffer(chain_buf, line_size);
	    val_buf = line_buf + key_size + 2;
	    break;
	}
    }

    key_buf = split_chain_buffer(chain_buf, key_size);
    memcpy(key_buf, line_buf, key_size);
    header->__fields = hash_insert(header->__fields, key_buf, val_buf, key_size);

    return header->__fields;
}

static void format_string(char *string, size_t size, char *fmt, ...)
{
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   vsnprintf(string, size, fmt, arg_ptr);
   va_end(arg_ptr);
}

static char *create_http_time(time_t *t, char *buf) {
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

#define MAX_HTTP_HEADER_SIZE 65536

char *create_http_response_header(http_header_t *h, char *dst_buf) {
    char *fmt =								\
	"HTTP/1.1 200 OK\r\n"						\
	"Server: rex\r\n" \
	"Content-Length: %zu\r\n" \
	"Date: %s\r\n"				\
	"Content-Type: text/html; charset=UTF-8\r\n"			\
	CRLF;
    char buf[MAX_HTTP_HEADER_SIZE], http_time_buf[64];
    size_t buf_size = sizeof(buf);
    time_t now;

    now = time(&now);

    create_http_time(&now, http_time_buf);

    format_string(dst_buf, buf_size, fmt, h->size, http_time_buf);

    return dst_buf;
}

static char *get_http_method(http_header_t *h) {
    char *m;

    switch (h->method) {

    case HTTP_REQUEST_METHOD_GET:

	m = "GET";
	break;

    default:

	m = "GET";
    };


    return m;
}

char *create_http_request_header_string(http_header_t *h, char *dst_buf) {
    char *fmt, buf[MAX_HTTP_HEADER_SIZE];

    fmt =
	/* verb */
	"%s "					\
	/* path  */
	"%s "					\
	/* version */
	"HTTP/1.1"				\
	CRLF					\
	"Host: %s"				\
	CRLF					\
	CRLF;

    format_string(dst_buf, sizeof(buf), fmt, get_http_method(h), h->path, "localhost");

    return dst_buf;
}

http_request_parse_result_t *create_http_request_parse_result(region_t *r) {
    http_header_t               *header;
    http_request_parse_result_t *request;

    r = ralloc(r, sizeof(http_request_parse_result_t));
    if (r == NULL) {
	return NULL;
    }

    request = (http_request_parse_result_t *)r->data;
    if (request == NULL) {
	return NULL;
    }

    r = init_http_header(r);
    if (r == NULL) {
	return NULL;
    }

    header = r->data;
    request->header = header;
    request->r = r;

    return request;
}

static char *parse_path(char *path_and_version, __buffer_t *chain_buf) {
    int  count;
    char ch, *path, *p;

    for (p = path_and_version, ch = *p, count = 0; ch != '\0'; p++, ++count) {

	ch = *p;

	if (ch == ' ') {
	    break;
	}
    }

    path = split_chain_buffer(chain_buf, count);

    memcpy(path, path_and_version, count);

    return path;
}

static rex_int parse_http_request_start(http_header_t *header, const char *line_buf, __buffer_t *chain_buf) {
    enum {
	start = 0,
	method,
	http_path,
	http_version,
	http_path_and_version,
    } state;

    state = start;

    char *p, ch, *request_start, *path, *path_and_version;
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
	    } else { /* todo */ }

	    break;

	case http_path:

	    path_and_version = request_start + (p - line_buf);
	    path = parse_path(path_and_version, chain_buf);
	    header->path = path;

	    state = http_version;

	    break;

	case http_version:
	    break;

	default:
	    goto error;

	}
    }

    if (header->method == 0) {
	goto error;
    }

    return REX_OK;

 error:
    return REX_ERROR;
}


#define MAX_HTTP_HEADER_LINE_BUFFER_SIZE 4096

#define LF '\n'
#define CR '\r'

rex_int
parse_http_request(http_header_t *header, __buffer_t *reqb, __buffer_t *chain_buf) {
    enum {
	start = 0,
	almost_done,
	done
    } state;

    char line[MAX_HTTP_HEADER_LINE_BUFFER_SIZE], ch, *p;
    int line_length = 0;
    rex_int parsed_status;

    state = start;

    for (p = reqb->pos; p < reqb->end; p++) {

	ch = *p;

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
	    if (state == almost_done && line_length <= MAX_HTTP_HEADER_LINE_BUFFER_SIZE) {

		line[line_length] = '\0';

		parsed_status = parse_http_request_start(header, line, chain_buf);

		if (parsed_status != REX_OK) {
		    goto error;
		}

		state = done;
	    } else { /* TODO */ }

	    break;

	default:
	    line[line_length] = ch;
	    ++line_length;

	}

	if (state == done) {
	    break;
	}
    }

    state = start;

    char *rest, *rest_end;
    rest = reqb->pos;
    rest += line_length;
    rest_end = rest + sizeof(char *) * line_length;

    line_length = 0;
    for (p = rest; p < rest_end; ++p) {

	ch = *p;

	switch (ch) {

	case CR:
	    state = almost_done;
	    break;

	case LF:
	    if (state == almost_done && line_length <= MAX_HTTP_HEADER_LINE_BUFFER_SIZE && line_length > 0) {
		char *line_buf;

		line_buf = split_chain_buffer(chain_buf, line_length);
		memcpy(line_buf, line, line_length);
		line_buf[line_length] = '\0';
		parse_http_request_meta_header_line(line_buf, line_length, header, chain_buf);

	    } else { /* TODO */ }

	    memset(line, 0, MAX_HTTP_HEADER_LINE_BUFFER_SIZE);
	    line_length = 0;
	    state = done;
	    break;

	default:
	    line[line_length] = ch;
	    ++line_length;

	}
    }

    return REX_OK;

 error:
    return REX_ERROR;
}

rex_hash_entry_t *fetch_header_field(char *key, size_t key_len, http_header_t *header) {
    rex_hash_entry_t *entry;

    entry = find_hash_entry(header->__fields, key, key_len);
    if (entry == NULL) {
	return NULL;
    }

    return entry;
}

region_t *init_http_header(region_t *r) {
    http_header_t *header;
    rex_hash_table_t *hash_table;

    r = init_hash_table(r);
    if (r == NULL) {
	return NULL;
    }
    hash_table = r->data;

    r = ralloc(r, sizeof(http_header_t));
    if (r == NULL) {
	return NULL;
    }

    header = r->data;
    header->path = NULL;
    header->size = 0;
    header->method = 0;
    header->version = 0;
    header->r = r;
    header->__fields = hash_table;

    return r;
}
