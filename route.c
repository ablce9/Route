#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <uv.h>

#include "./buffer.h"
#include "./map.h"
#include "./http.h"

#define CRLF "\r\n"

typedef struct {
    http_response_payload_t *response;
    http_request_payload_t *request;
} request_data_t;

static uv_loop_t *loop;

static void close_cb(uv_handle_t* handle) {
    // if (handle->data) {
    //	__map_t *map;
    //	int i = 0;
    //	request_data_t *data = handle->data;
    //	http_response_payload_t *response = data->response;
    //	http_request_payload_t *request = data->request;
    //	__buffer_t *map_value;
    //
    //	// while ((map = request->header->meta[i])) {
    //	//
    //	//     // map key
    //	//     free(map->key->bytes);
    //	//     free(map->key);
    //	//
    //	//     // map value
    //	//     map_value = (__buffer_t*)map->value;
    //	//     free(map_value->bytes);
    //	//     free(map_value);
    //	//
    //	//     free(map);
    //	//
    //	//     ++i;
    //	// }
    //	// free(request->header->start->bytes);
    //	// free(request->header->start);
    //	// free(request->header);
    //	// free(request);
    //	//
    //	// free(response->header);
    //	// free(response->body);
    //	// free(response);
    //	//
    //	// free(data);
    // };
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

static size_t make_file_buffer(char *out) {
    const char *filename = "index.html";
    uv_fs_t stat_req;
    int result = uv_fs_stat(NULL, &stat_req, filename, NULL);
    if (result != 0) {
	return -1;
    }

    size_t file_size = ((uv_stat_t*)&stat_req.statbuf)->st_size;

    memset(out, 0, file_size);

    uv_fs_t open_req, read_req, close_req;
    uv_buf_t read_file_data = {
	.base = out,
	.len = file_size,
    };

    uv_fs_open(NULL, &open_req, filename, O_RDONLY, 0, NULL);
    uv_fs_read(NULL, &read_req, open_req.result, &read_file_data, 1, 0, NULL);
    uv_fs_close(NULL, &close_req, open_req.result, NULL);

    return file_size;
}

static void write_cb(uv_write_t *request, int status) {
    if (status < 0) {
	fprintf(stderr, "write error %i\n", status);
    }

    free(request);
}

static void invalid_request_close_cb(uv_handle_t* handle) {
    free(handle);
}

static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* request_buf) {
    region_t *r;
    size_t response_size;
    __buffer_t *chain_buf;
    char *header_buffer, *file_buf;

    if (nread <= 0) {
	uv_close((uv_handle_t*)handle, invalid_request_close_cb);
	return;
    }

    r = (region_t *)handle->data;

    uv_buf_t response_vec[2];

    chain_buf = create_chain_buffer(r, sizeof(char *) * 4049);

    file_buf = chain_buf->pos;
    response_size = make_file_buffer(file_buf);
    chain_buf->pos += sizeof(char *) * response_size;

    header_buffer = build_http_header(response_size);

    // set header
    response_vec[0].base = header_buffer;
    response_vec[0].len = strlen(header_buffer);

    // set body
    response_vec[1].base = file_buf;
    response_vec[1].len = response_size;

    http_response_payload_t response_payload = {
	.header = header_buffer,
	.body = file_buf
    };

    http_request_payload_t *request_payload = new_http_request_payload();
    __buffer_t *http_request_header_buf = alloc_new_buffer(request_buf->base);

    route_int parsed_status = parse_http_request_header(request_payload->header, http_request_header_buf);
    if (parsed_status != ROUTE_OK) {
	goto error;
    }

    request_data_t *data_p = calloc(1, sizeof(request_data_t));
    request_data_t data = {
	.response = &response_payload,
	.request = request_payload
    };
    *data_p = data;
    handle->data = (void*)data_p;

    uv_write_t *writer = (uv_write_t*)malloc(sizeof(uv_write_t));

    uv_write(writer, handle, response_vec, 2, write_cb);

    uv_close((uv_handle_t*)handle, close_cb);

    free(http_request_header_buf->bytes);
    free(http_request_header_buf);

    free(request_buf->base);

    return;

 error:
    printf("[debug] invalid requests. closing connection.\n");
    uv_close((uv_handle_t*)handle, invalid_request_close_cb);

    free(request_buf->base);
}

void on_new_connection(uv_stream_t* server, int status) {
    region_t *r;
    uv_tcp_t *client;

    if (status < 0) {
	fprintf(stderr, "new connection error: %s\n", uv_strerror(status));
	return;
    }

    r = create_region();

    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    client->data = (void *)r;

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
