#include <unistd.h>
#include <string.h>

#include "picohttpparser/picohttpparser.h"
#include "mongoose/mongoose.h"
#include "http_handler.h"
#include "socket_io.h"
#include "logging.h"

struct handler_ctx *handler_init()
{
    struct handler_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        perror("malloc");
        return NULL;
    }

    ctx->bufsize = BUFSIZE;
    ctx->buf = malloc(ctx->bufsize);
    if (!ctx->buf) {
        perror("malloc");
        free(ctx);
        return NULL;
    }

    return ctx;
}

void handler_destroy(struct handler_ctx *ctx)
{
    free(ctx->buf);
    free(ctx);
}

int http_handler(int sock_client, struct handler_ctx *ctx)
{
    struct http_message request;

    int nread;
    nread = socket_read(sock_client, ctx->buf, ctx->bufsize);
    if (nread < 0) {
        return -1;
    }
    debug("Read %d\n", nread);

    nread = mg_parse_http(ctx->buf, nread, &request, 1);
    if (nread <= 0) {
        return -1;
    }
    debug("Successfully parsed request\n");

    if (strncmp(request.method.p, GET, request.method.len) == 0) {
        // Pretend to do some work for 100ms
        // usleep(100000);
        return socket_write(sock_client, RESPONSE_BODY, strlen(RESPONSE_BODY));
    } else {
        return -2;
    }
}

int http_handler_loop(int sock_client)
{
    struct http_message request;
    struct handler_ctx *ctx;
    int rc = 0;
    int nread;

    do {
        ctx = handler_init();

        nread = socket_read(sock_client, ctx->buf, ctx->bufsize);
        if (nread < 0) {
            rc = -1;
            break;
        }
        debug("Read %d\n", nread);

        /*
        const char *method, *path;
        int pret, minor_version;
        struct phr_header headers[100];
        size_t method_len, path_len, num_headers;

        pret = phr_parse_request(ctx->buf, nread, &method, &method_len, &path, &path_len,
                         &minor_version, headers, &num_headers, 0);
        */

        nread = mg_parse_http(ctx->buf, nread, &request, 1);
        if (nread <= 0) {
            rc = -2;
            break;
        }
        debug("Successfully parsed request\n");

        if (strncmp(request.method.p, GET, request.method.len) == 0) {
        //if (strncmp(method, GET, method_len) == 0) {
            // Pretend to do some work for 100ms
            // usleep(100000);
            if (socket_write(sock_client, RESPONSE_BODY, strlen(RESPONSE_BODY))) {
                rc = -3;
                break;
            }
        } else {
            rc = -4;
            break;
        }
        handler_destroy(ctx);

    } while (nread > 0);

    handler_destroy(ctx);
    return rc;
}
