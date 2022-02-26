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
    region_t    *r;
    __buffer_t  *rb;
} request_context_t;

static uv_loop_t *loop;

static void close_cb(uv_handle_t* handle) {
    region_t          *r;
    request_context_t *ctx;

    ctx = (request_context_t *)handle->data;
    r = (region_t *)ctx->r;

    destroy_regions(r);
    free(handle);
}

static size_t make_file_buffer(char *buf) {
    const char *filename = "index.html";
    uv_fs_t stat_req;
    int result = uv_fs_stat(NULL, &stat_req, filename, NULL);

    if (result != 0) {
	return -1;
    }

    size_t file_size = ((uv_stat_t*)&stat_req.statbuf)->st_size;
    char fbuf[file_size];
    memset(fbuf, 0, file_size);

    uv_fs_t open_req, read_req, close_req;
    uv_buf_t read_file_data = {
	.base = fbuf,
	.len = file_size,
    };

    uv_fs_open(NULL, &open_req, filename, O_RDONLY, 0, NULL);
    uv_fs_read(NULL, &read_req, open_req.result, &read_file_data, 1, 0, NULL);
    uv_fs_close(NULL, &close_req, open_req.result, NULL);

    fbuf[file_size] = '\0';
    memcpy(buf, fbuf, file_size);

    return file_size;
}

static void write_cb(uv_write_t *request, int status) {
    if (status < 0) {
	fprintf(stderr, "write error %i\n", status);
    }

    free(request);
}

static void invalid_request_close_cb(uv_handle_t* handle) {
    request_context_t *ctx;
    region_t *r;

    ctx = (request_context_t *)handle->data;
    r = (region_t *)ctx->r;

    destroy_regions(r);
    free(handle);
}

static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* request_buf) {
    char                       *file_buf, header_buffer[4096];
    size_t                     response_size;
    __map_t                    *map;
    region_t                   *r;
    uv_buf_t                   response_vec[2];
    route_int                  parsed_status;
    __buffer_t                 *chain_buf, reqb;
    uv_write_t                 *writer;
    http_header_t              header;
    request_context_t         *ctx;
    http_request_payload_t    *request_payload;

    if (nread <= 0) {
	printf("n=%ld,b=%s\n", nread, request_buf->base);
	uv_close((uv_handle_t*)handle, invalid_request_close_cb);
	return;
    }

    ctx = (request_context_t *)handle->data;
    r = ctx->r;

    ctx->rb->pos += sizeof(char *) * nread;

    chain_buf = ctx->rb;

    response_size = make_file_buffer(chain_buf->pos);
    BUFFER_MOVE(file_buf, chain_buf->pos, response_size);

    header.size = response_size;

    make_http_response_header(&header, header_buffer);

    response_vec[0].base = header_buffer;
    response_vec[0].len = strlen(header_buffer);

    response_vec[1].base = file_buf;
    response_vec[1].len = response_size;

    map = (__map_t *)init_map(r, 1024);
    r = map->r;

    request_payload = (http_request_payload_t *)new_http_request_payload(r);
    r = request_payload->r;

    reqb.pos = request_buf->base;
    reqb.end = reqb.pos + (sizeof(char *) * nread);

    parsed_status = parse_http_request(request_payload->header, &reqb, chain_buf, map);

    if (parsed_status != ROUTE_OK) {
	goto error;
    }

    writer = (uv_write_t*)malloc(sizeof(uv_write_t));

    ctx->r = r;
    ctx->rb = chain_buf;
    handle->data = (void *)ctx;

    uv_write(writer, handle, response_vec, 2, write_cb);

    uv_close((uv_handle_t*)handle, close_cb);
    return;

 error:
    printf("[debug] invalid requests. closing connection.\n");
    uv_close((uv_handle_t*)handle, invalid_request_close_cb);

    free(request_buf->base);
}

static request_context_t *make_request_context(region_t *r) {
    region_t          *new_region;
    request_context_t *ctx;

    new_region = ralloc(r, sizeof(request_context_t));
    if (new_region == NULL) {
	return NULL;
    }

    ctx = (request_context_t *)new_region->data;

    ctx->r = new_region;
    ctx->rb = NULL;

    return ctx;
}

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    region_t          *r;
    __buffer_t        *rb;
    request_context_t *ctx;

    r = create_region();

    rb = create_chain_buffer(r, 1024 * 16);

    ctx = make_request_context(rb->r);
    ctx->rb = rb;

    buf->base = rb->pos;
    buf->len = suggested_size;

    handle->data = (void *)ctx;
}

static void on_new_connection(uv_stream_t* server, int status) {
    uv_tcp_t *client;

    if (status < 0) {
	fprintf(stderr, "new connection error: %s\n", uv_strerror(status));
	return;
    }

    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

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
