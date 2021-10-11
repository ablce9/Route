#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <uv.h>

#include "buffer.h"


#define LF '\n'
#define CR '\r'
#define CRLF "\r\n"

#define MAX_HTTP_HEADER_LINE_BUFFER_SIZE 4096

#define HTTP_REQUEST_METHOD_GET  0x1
#define HTTP_REQUEST_METHOD_PUT  0x2
#define HTTP_REQUEST_METHOD_POST 0x4
#define HTTP_REQUEST_METHOD_HEAD 0x5

#define ROUTE_OK 0
#define ROUTE_ERROR -1


typedef struct {
    char *header;
    char *body;
} http_response_payload_t;

typedef struct {
    __buffer_t *key;
    void       *value;
} __hash_t;

typedef struct {
    __buffer_t  *start;
    uint8_t      method;
    int          start_length;
    __hash_t    *meta[1024];
} http_request_header_t;

typedef struct {
    http_request_header_t *header;
    // todo: add body
} http_request_payload_t;

typedef struct {
    http_response_payload_t *response;
    http_request_payload_t *request;
} request_data_t;

typedef intptr_t route_int;

static http_request_header_t *new_http_request_header(void);
static route_int parse_http_request_header(http_request_header_t *header, const __buffer_t *header_buf);
static route_int parse_http_request_header_start(http_request_header_t *header, const char *line_buf, const int line_length);
static http_response_payload_t* new_http_response_payload(char *header, char *body);
static http_request_payload_t* new_http_request_payload(void);
static __hash_t *new_hash(void);
static __hash_t *parse_http_request_meta_header_line(const char *line_buf);
static void prepare_http_header(size_t content_length, uv_buf_t *buf);


static uv_loop_t *loop;

static void close_cb(uv_handle_t* handle) {
    if (handle->data) {
	__hash_t *hash;
	int i = 0;
	request_data_t *data = handle->data;
	http_response_payload_t *response = data->response;
	http_request_payload_t *request = data->request;

	while ((hash = request->header->meta[i])) {
	    free(hash->key);
	    free(hash->value);
	    free(hash);
	    ++i;
	}
	free(request->header->start->bytes);
	free(request->header->start);
	free(request->header);
	free(request);

	free(response->header);
	free(response->body);
	free(response);

	free(data);
    };
    free(handle);
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = calloc(suggested_size, sizeof(char));
    buf->len = suggested_size;
}

static void format_string(char *string, size_t size, char *fmt, ...)
{
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   vsnprintf(string, size, fmt, arg_ptr);
   va_end(arg_ptr);
}

static char *build_http_header(size_t content_length) {
    char *fmt =								\
	"HTTP/1.1 200 OK\n"						\
	"Server: route\n" \
	"Content-Length: %zu\n" \
	"Date: Sun, 18 Apr 2021 04:13:15 GMT\n"				\
	"Content-Type: text/html; charset=UTF-8\n"			\
	CRLF;
    char buf[4049];

    memset(buf, 0, sizeof(buf));
    format_string(buf, sizeof(buf), fmt, content_length);

    char *header_buffer = malloc(sizeof(char*)*sizeof(buf));
    memcpy(header_buffer, buf, sizeof(buf));

    return header_buffer;
}

static void prepare_http_header(size_t content_length, uv_buf_t *buf) {
    char *header_buffer = build_http_header(content_length);

    buf->base = header_buffer;
    buf->len = strlen(header_buffer);
}

static int make_file_buffer(uv_buf_t *file_buffer) {
    const char *filename = "index.html";
    uv_fs_t stat_req;
    int result = uv_fs_stat(NULL, &stat_req, filename, NULL);
    if (result != 0) return 0;

    size_t file_size = ((uv_stat_t*)&stat_req.statbuf)->st_size;
    char *read_file_buffer = malloc(sizeof(char)*file_size);
    memset(read_file_buffer, 0, file_size);

    uv_fs_t open_req, read_req, close_req;
    uv_buf_t read_file_data = {
	.base = read_file_buffer,
	.len = file_size,
    };

    uv_fs_open(NULL, &open_req, filename, O_RDONLY, 0, NULL);
    uv_fs_read(NULL, &read_req, open_req.result, &read_file_data, 1, 0, NULL);
    uv_fs_close(NULL, &close_req, open_req.result, NULL);

    *file_buffer = read_file_data;
    return file_size;
}

static void write_cb(uv_write_t *request, int status) {
    if (status < 0) {
	fprintf(stderr, "write error %i\n", status);
    }

    free(request);
}

static uv_buf_t* new_http_iovec() {
    uv_buf_t* iovec = malloc(sizeof(uv_buf_t)*2);
    return iovec;
}

static http_response_payload_t *new_http_response_payload(char *header, char *body) {
    http_response_payload_t *http_payload = malloc(sizeof(http_response_payload_t));
    http_response_payload_t hrp = {
	.header = header,
	.body = body
    };
    *http_payload = hrp;
    return http_payload;
}

static http_request_payload_t *new_http_request_payload(void) {
    http_request_header_t *header = new_http_request_header();
    http_request_payload_t *request = calloc(1, sizeof(http_request_payload_t));

    http_request_payload_t req = {
	.header = header
    };
    *request = req;

    return request;
}

static __hash_t *new_hash(void) {
    __hash_t *hash = calloc(1, sizeof(__hash_t));

    hash->key = NULL;
    hash->value = NULL;

    return hash;
}

static __hash_t *parse_http_request_meta_header_line(const char *line_buf) {
    __buffer_t *buf = alloc_new_buffer(line_buf);
    __hash_t *hash = new_hash();
    char ch, value_buf[2048], *p;
    int value_len = 0;

    for (p = buf->pos; p < buf->end; p++) {

	ch = *p;

	if (ch == ':') {
	    hash->key = copyn_buffer(buf, p - buf->start);
	    break;
	}
    }

    ++p;
    buf->pos += p - buf->start;

    for (p = buf->pos; p < buf->end; p++) {

	ch = *p;

	if (ch != ' ') {
	    value_buf[value_len] = ch;
	    ++value_len;
	}
    }

    hash->value = malloc(sizeof(char)*(value_len));
    value_buf[value_len] = '\0';
    memcpy(hash->value, (void*)value_buf, value_len);

    free(buf->bytes);
    free(buf);

    return hash;
}

#define str4_comp(str, c0, c1, c2, c3) *(uint32_t *) str == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

static route_int
parse_http_request_header_start(http_request_header_t *header, const char *line_buf, const int line_length) {
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
		    if (str4_comp(request_start, 'G', 'E', 'T', ' ')) {
			header->method = HTTP_REQUEST_METHOD_GET;
			state = http_path;
			break;
		    }

		    if (str4_comp(request_start, 'P', 'U', 'T', ' ')) {
			header->method = HTTP_REQUEST_METHOD_PUT;
			state = http_path;
			break;
		    }

		    goto error;

		case 4:
		    if (str4_comp(request_start, 'P', 'O', 'S', 'T')) {
			header->method = HTTP_REQUEST_METHOD_POST;
			state = http_path;
			break;
		    }

		    if (str4_comp(request_start, 'H', 'E', 'A', 'D')) {
			header->method = HTTP_REQUEST_METHOD_POST;
			state = http_path;
			break;
		    }

		    goto error;

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

static http_request_header_t *new_http_request_header(void) {
    http_request_header_t *header = calloc(1, sizeof(http_request_header_t));

    header->start = NULL;
    header->method = 0;
    header->start_length = 0;

    return header;
}

static route_int
parse_http_request_header(http_request_header_t *header, const __buffer_t *header_buf) {
    enum {
	start = 0,
	almost_done,
	done
    } state;

    char line[MAX_HTTP_HEADER_LINE_BUFFER_SIZE], ch;
    int i = 0, line_length = 0, hash_index = 0;
    __hash_t *hash;

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
		parsed_status = parse_http_request_header_start(header, line, line_length);

		if (parsed_status != ROUTE_OK) {
		    goto error;
		}

		state = done;
		break;
	    }

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
		hash = parse_http_request_meta_header_line(line);
		header->meta[hash_index] = hash;
		++hash_index;
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

static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* request_buf) {
    uv_buf_t *response_vec = new_http_iovec();
    prepare_http_header(make_file_buffer(&response_vec[1]), &response_vec[0]);
    http_response_payload_t *response_payload = new_http_response_payload(response_vec[0].base, response_vec[1].base);

    http_request_payload_t *request_payload = new_http_request_payload();
    __buffer_t http_request_header_buf = {
	.size = request_buf->len,
	.bytes = request_buf->base,
    };
    route_int parsed_status = parse_http_request_header(request_payload->header, &http_request_header_buf);

    if (parsed_status != ROUTE_OK) {
	goto error;
    }

    request_data_t *data_p = calloc(1, sizeof(request_data_t));
    request_data_t data = {
	.response = response_payload,
	.request = request_payload
    };
    *data_p = data;
    handle->data = (void*)data_p;

    uv_write_t *writer = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_write(writer, handle, response_vec, 2, write_cb);

    uv_close((uv_handle_t*)handle, close_cb);

    free(request_buf->base);
    free(response_vec);

    return;

 error:
    printf("[debug] invalid requests. closing connection.\n");
    uv_close((uv_handle_t*)handle, close_cb);

    free(request_buf->base);
    free(response_vec);
}

void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
	fprintf(stderr, "new connection error: %s\n", uv_strerror(status));
	return;
    }

    uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    uv_tcp_init(loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
	uv_read_start((uv_stream_t*)client, alloc_buffer, read_cb);
    } else {
	printf("accept failed\n");
    }
}

int main() {
    uv_tcp_t server;

    loop = uv_default_loop();
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;

    uv_ip4_addr("0.0.0.0", 2020, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);

    int r = uv_listen((uv_stream_t*) &server, 1024, on_new_connection);
    if (r) {
	fprintf(stderr, "Listen error %s\n", uv_strerror(r));
	return 1;
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}
