#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <uv.h>

#include "./buffer.h"
#include "./hash.h"
#include "./http.h"

#define CRLF "\r\n"

static uv_loop_t *loop;

typedef struct {
    char           *addr;
    unsigned short port;
} route_dest_t;

typedef struct {
    char         *name;
    char         *path;
    route_dest_t *dest;
} router_t;

typedef struct {
    router_t *routers;
} server_config_t;

typedef struct {
    region_t        *r;
    __buffer_t      *rb;
    uv_stream_t     *uv_stream;
    http_header_t   *header;
    server_config_t *config;
} request_context_t;

typedef struct {
    uv_tcp_t            *uv_socket;
    uv_connect_t        *uv_con;
    uv_loop_t           *uv_loop;
    struct sockaddr_in  dest_address;
} connection_t;

static void write_cb(uv_write_t *request, int status);
static void close_cb(uv_handle_t* handle);
static void close_proxy_connection_cb(uv_handle_t* handle);

static region_t *init_connection(region_t *r, connection_t *c) {
    uv_tcp_t            *uv_socket;
    uv_connect_t        *uv_con;

    r = ralloc(r, sizeof(uv_connect_t));
    uv_con = (uv_connect_t *)r->data;

    r = ralloc(r, sizeof(uv_tcp_t));
    uv_socket = (uv_tcp_t *)r->data;

    c->uv_con    = uv_con;
    c->uv_socket = uv_socket;

    uv_tcp_init(loop, c->uv_socket);

    return r;
}

static region_t *make_response_writer(region_t *r) {
    region_t   *new_region;

    new_region = ralloc(r, sizeof(uv_write_t));
    if (new_region == NULL) {
	return NULL;
    }

    return new_region;
}

static void alloc_proxy_buffer_handler(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    request_context_t *ctx;

    ctx = (request_context_t *)handle->data;

    buf->base = ctx->rb->pos;
    buf->len  = suggested_size;
}

static void proxy_read_handler(uv_stream_t *handle, ssize_t read_count, const uv_buf_t *req_buf) {
    char              header_buf[4096];
    region_t          *r;
    uv_write_t        *writer;
    http_header_t      header;
    request_context_t *ctx;

    ctx = handle->data;

    uv_handle_t *proxy_handle = (uv_handle_t *)ctx->uv_stream;

    proxy_handle->data = (void *)ctx;

    if (read_count == UV_EOF && uv_is_closing((uv_handle_t *)handle) == 0) {
      uv_close((uv_handle_t *)handle, close_proxy_connection_cb);
      return;
    }

    r = ctx->r;
    r = ralloc(r, sizeof(uv_write_t));
    writer = (uv_write_t *)r->data;

    ctx->r = r;
    ctx->uv_stream->data = ctx;

    header.size = read_count;
    create_http_response_header(&header, header_buf);

    uv_buf_t res_vec[1] = {
	{ .base = req_buf->base, .len = read_count },
    };

    writer->data = (void *)ctx;

    uv_write(writer, ctx->uv_stream, res_vec, 1, NULL);
}

static void proxy_connection_handler(uv_connect_t *c, int status) {
    int               read_status;
    char              header_buffer[65536], *header;
    uv_buf_t          response_vec[1];
    region_t          *r;
    uv_write_t        *writer;
    request_context_t *ctx;

    ctx = (request_context_t *)c->data;

    if (status < 0) {
	printf("connection error: %s\n", uv_strerror(status));
	goto error;
    }

    r = ctx->r;
    r = make_response_writer(r);
    writer = (uv_write_t*)r->data;

    header = create_http_request_header_string(ctx->header, header_buffer);

    response_vec[0].base = header;
    response_vec[0].len = strnlen(header, sizeof(header_buffer));

    ctx->r = r;
    c->handle->data = ctx;

    uv_write(writer, c->handle, response_vec, 1, write_cb);
    read_status = uv_read_start(c->handle, alloc_proxy_buffer_handler, proxy_read_handler);

    if (read_status < 0) {
	printf("[error] proxy read %s\n", uv_strerror(read_status));
    }

    return;

 error:
      uv_close((uv_handle_t *)ctx->uv_stream, close_cb);
}

static void close_cb(uv_handle_t* handle) {
  request_context_t *ctx;

  ctx = (request_context_t *)handle->data;

  destroy_regions(ctx->r);
}

static void close_proxy_connection_cb(uv_handle_t* handle) {
    request_context_t *ctx;

    ctx = (request_context_t *)handle->data;

    if (ctx->uv_stream == NULL) {
      return;
    }

    if (uv_is_closing((uv_handle_t *)ctx->uv_stream) == 0) {
      uv_close((uv_handle_t *)ctx->uv_stream, close_cb);
    }
}

static size_t make_file_buffer(char *buf) {
    size_t     file_size;
    uv_fs_t    stat_req, open_req, read_req, close_req;
    const char *filename;

    filename = "index.html";

    if (uv_fs_stat(NULL, &stat_req, filename, NULL) != 0) {
	return -1;
    }

    file_size = ((uv_stat_t*)&stat_req.statbuf)->st_size;
    char fbuf[file_size];
    memset(fbuf, 0, file_size);

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
	printf("write error: %s\n", uv_strerror(status));
    }
}

static void handle_request(uv_stream_t *handle, ssize_t nread, const uv_buf_t *request_buf) {
    char                           *file_buf, header_buffer[4096];
    size_t                         response_size;
    rex_int                        parsed_status;
    region_t                       *r;
    __buffer_t                     *chain_buf, reqb;
    http_header_t                  header;
    rex_hash_entry_t               *hash_entry;
    rex_hash_table_t               *hash_table;
    request_context_t              *ctx;
    http_request_parse_result_t    *http_request_parse_result;

    if (nread < 0) {
	printf("invalid request, closing a connection\n");
	uv_close((uv_handle_t*)handle, close_proxy_connection_cb);
	return;
    }

    ctx = (request_context_t *)handle->data;
    r = ctx->r;

    ctx->rb->pos += sizeof(char *) * nread;

    chain_buf = ctx->rb;

    // response_size = make_file_buffer(chain_buf->pos);
    // file_buf = split_chain_buffer(chain_buf, response_size);
    //
    // header.size = response_size;
    //
    // create_http_response_header(&header, header_buffer);

    // response_vec[0].base = header_buffer;
    // response_vec[0].len = strlen(header_buffer);
    //
    // response_vec[1].base = file_buf;
    // response_vec[1].len = response_size;

    r = init_hash_table(r);
    hash_table = (rex_hash_table_t *)r->data;

    http_request_parse_result = (http_request_parse_result_t *)create_http_request_parse_result(r);
    r = http_request_parse_result->r;

    reqb.pos = request_buf->base;
    reqb.end = reqb.pos + (sizeof(char *) * nread);

    parsed_status = parse_http_request(http_request_parse_result->header, &reqb, chain_buf, hash_table);

    if (parsed_status != REX_OK) {
	goto error;
    }

    int                 connection_status;
    connection_t        c;
    struct sockaddr_in  dest_address;

    r = init_connection(r, &c);

    ctx->uv_stream = handle;
    ctx->header    = http_request_parse_result->header;
    ctx->r         = r;
    ctx->rb        = chain_buf;

    handle->data = (void *)ctx;
    c.uv_con->data = (void *)ctx;

    uv_ip4_addr("127.0.0.1", 80, &dest_address);
    connection_status = uv_tcp_connect(
				       c.uv_con,
				       c.uv_socket,
				       (const struct sockaddr *)&dest_address,
				       proxy_connection_handler
				       );

    if (connection_status < 0) {
	printf("connection error: %s\n", uv_strerror(connection_status));
	uv_close((uv_handle_t *)handle, close_proxy_connection_cb);
    }

    return;

 error:
    printf("[debug] invalid requests. closing connection.\n");
    uv_close((uv_handle_t*)handle, close_proxy_connection_cb);
}

static region_t *make_request_context(region_t *r) {
    region_t          *new_region;
    request_context_t *ctx;

    new_region = ralloc(r, sizeof(request_context_t));
    if (new_region == NULL) {
	return NULL;
    }

    ctx = (request_context_t *)new_region->data;

    ctx->r         = new_region;
    ctx->rb        = NULL;
    ctx->config    = NULL;
    ctx->uv_stream = NULL;
    new_region->data = (void *)ctx;

    return new_region;
}

static void proxy_request_handler(uv_stream_t* stream, ssize_t buffer_size, const uv_buf_t* request_buffer) {
    int                 connection_status;
    region_t            *r;
    connection_t        c;
    request_context_t   *ctx;
    struct sockaddr_in  dest_address;

    if (buffer_size < 0) {
	printf("error occurred on proxy_request_handler\n");
	uv_close((uv_handle_t*)stream, close_proxy_connection_cb);
	return;
    }

    ctx = (request_context_t *)stream->data;
    r = ctx->r;

    ctx->rb->pos += sizeof(char *) * buffer_size;

    r = init_connection(r, &c);

    uv_ip4_addr("127.0.0.1", 8000, &dest_address);
    connection_status = uv_tcp_connect(c.uv_con, c.uv_socket, (const struct sockaddr *)&dest_address, proxy_connection_handler);
    if (connection_status < 0) {
	printf("connection error: %s\n", uv_strerror(connection_status));
    }

    ctx->r = r;
    stream->data = (void *)ctx;
}

static void alloc_request_buffer_handler(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    region_t          *r;
    __buffer_t        *rb;
    request_context_t *ctx;

    ctx = (request_context_t *)handle->data;

    r = ctx->r;
    r = create_chain_buffer(r, 1024 * 24);
    rb = (__buffer_t *)r->data;

    ctx->r  = r;
    ctx->rb = rb;

    buf->base = rb->pos;
    buf->len  = suggested_size;

    handle->data = (void *)ctx;
}

static void handle_new_connection(uv_stream_t* server, int status) {
    region_t          *r;
    uv_tcp_t          *client;
    server_config_t   *config;
    request_context_t *ctx;

    if (status < 0) {
	fprintf(stderr, "new connection error: %s\n", uv_strerror(status));
	return;
    }

    config = (server_config_t *)server->data;

    r = create_region();

    r = make_request_context(r);
    ctx = (request_context_t *)r->data;

    r = ralloc(r, sizeof(uv_tcp_t));
    client = (uv_tcp_t *)r->data;

    ctx->r      = r;
    ctx->config = config;

    client->data = (void *)ctx;

    uv_tcp_init(loop, client);

    if (uv_accept(server, (uv_stream_t *)client) == 0) {
	uv_read_start((uv_stream_t *)client, alloc_request_buffer_handler, handle_request);
    } else {
	printf("accept failed\n");
    }
}

int main() {
    int                 listen_status;
    uv_tcp_t            *server;
    region_t            *r;
    server_config_t     *config;
    struct sockaddr_in  addr;

    loop = uv_default_loop();

    r = create_region();
    r = ralloc(r, sizeof(uv_tcp_t));
    server = (uv_tcp_t *)r->data;

    r = ralloc(r, sizeof(server_config_t));
    config = (server_config_t *)r->data;

    route_dest_t dest = {
	.addr = "localhost",
	.port = 8000
    };

    router_t routers[] = {
	{
	    .name = "app1",
	    .path = "/",
	    .dest = &dest
	}
    };

    uv_tcp_init(loop, server);
    uv_ip4_addr("0.0.0.0", 2020, &addr);
    uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);

    config->routers = routers;
    server->data = (void *)config;

    listen_status = uv_listen((uv_stream_t*)server, 1024, handle_new_connection);

    if (listen_status) {
	fprintf(stderr, "Listen error %s\n", uv_strerror(listen_status));
	return 1;
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}
