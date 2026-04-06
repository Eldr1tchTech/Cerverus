#include "io_uring_helper.h"

#include "network/http/request.h"
#include "core/util/logger.h"
#include "network/http/response.h"

#include <netinet/in.h>
#include <string.h>

void uring_process_completions(server *srv)
{
    struct io_uring_cqe *cqe;

    while (io_uring_peek_cqe(&srv->ring, &cqe) == 0)
    {
        uring_context *ctx = cqe->user_data;

        switch (ctx->op_type)
        {
        case uring_op_type_accept:
            handle_accept_completion(cqe, ctx);
            break;
        case uring_op_type_recv:
            handle_recv_completion(cqe, ctx);
            break;
        case uring_op_type_send:
            handle_send_completion(cqe, ctx);
            break;
        case uring_op_type_openat:
            handle_openat_completion(cqe, ctx);
            break;
        case uring_op_type_close:
            handle_close_completion(ctx);
        default:
            break;
        }

        io_uring_cqe_seen(&srv->ring, cqe);
    }
}

void handle_accept_submission(server *srv)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&srv->ring);

    uring_context *ctx = cmem_alloc(memory_tag_io_uring, sizeof(uring_context));
    ctx->srv = srv;
    ctx->op_type = uring_op_type_accept;

    io_uring_prep_accept(sqe, srv->socket_fd, &ctx->client.addr, &ctx->client.addrlen, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&srv->ring);
}

void handle_accept_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
    if (cqe->res < 0)
    {
        cmem_free(memory_tag_io_uring, ctx);
        handle_accept_submission(ctx->srv);
        return;
    }

    // Logging code
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *addr = (struct sockaddr_in *)&ctx->client.addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    int port = ntohs(addr->sin_port);

    LOG_INFO("Connection from: %s:%d", ip, port);

    handle_accept_submission(ctx->srv);

    ctx->client.fd = cqe->res;
    handle_recv_submission(ctx);
}

// TODO: If I were to pass ring as an argument, does it reduce lookup time, since it will never leave the registers?
void handle_recv_submission(uring_context *ctx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->ring);
    ctx->op_type = uring_op_type_recv;

    io_uring_prep_recv(sqe, ctx->client.fd, ctx->request.buffer, BUFFER_SIZE, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->ring);
}

// TODO: Eventually should flush all requests within a read, not just the first one.
void handle_recv_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
    request_parse_state_context parse_ctx = request_parse(ctx->request.buffer, BUFFER_SIZE, ctx->request.request);  // TODO: Maybe use cqe->res for buffer size instead?

    switch (parse_ctx.type)
    {
    case request_parse_state_succeded:
        cmem_mcpy(ctx->request.buffer, ctx->request.buffer + parse_ctx.bytes_consumed, BUFFER_SIZE - parse_ctx.bytes_consumed);
        server_handle_request(ctx->srv, ctx->request, ctx->client.fd);
        break;
    case request_parse_state_invalid:
        handle_close_submission(ctx, ctx->client.fd);
        break;
    case request_parse_state_unfinished:
        cmem_mcpy(ctx->request.buffer, ctx->request.buffer + parse_ctx.bytes_consumed, BUFFER_SIZE - parse_ctx.bytes_consumed);
        handle_recv_submission(ctx);
    }
}

void handle_openat_submission(uring_context *ctx, char* path) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->ring);
    ctx->op_type = uring_op_type_openat;

    io_uring_prep_openat(sqe, 0, path, 0, O_RDONLY);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->ring);
}

void handle_openat_completion(struct io_uring_cqe* cqe, uring_context* ctx) {
    if (cqe->res <= 0)
    {
        LOG_ERROR("handle_openat_completion - Failed.");
        return;
    }
    
    ctx->file_fd = cqe->res;
}

void handle_send_submission(uring_context *ctx) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->ring);
    ctx->op_type = uring_op_type_send;

    if (!ctx->response.buffer) {
        ctx->response.buffer = response_serialize(ctx->response.response);  // TODO: Eventually have this fill the length.
        ctx->response.length = strlen(ctx->response.buffer);
        ctx->response.offset = 0;
    }

    io_uring_prep_send(sqe, ctx->client.fd, ctx->response.buffer + (sizeof(char) * ctx->response.offset), ctx->response.length, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->ring);
}

void handle_send_completion(struct io_uring_cqe *cqe, uring_context *ctx) {
    if (cqe->res < 0)
    {
        LOG_ERROR("error?");
        return;
    }
    
    ctx->response.offset += cqe->res;

    if (ctx->response.length > ctx->response.offset)
    {
        handle_send_submission(ctx);
        return;
    }
    
    handle_close_submission(&ctx->srv->ring, ctx->client.fd); // TODO: Please note that you need to take care of file desccriptors here
    // The best solution is probably to wrap the openat with an check to an internal file_fd cache.
}

void handle_sendfile_submission(uring_context *ctx) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->ring);
    ctx->op_type = uring_op_type_sendfile;

    io_uring_prep_splice(sqe, ctx->file_fd, 0, ctx->client.fd, 0, 0, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->ring);
}

void handle_sendfile_completion(struct io_uring_cqe *cqe, uring_context *ctx) {
    
}

void handle_close_submission(struct io_uring* ring, int fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    uring_close_context* ctx = cmem_alloc(memory_tag_io_uring, sizeof(uring_close_context));
    ctx->op_type = uring_op_type_close;

    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(ring);
}

void handle_close_completion(uring_context *ctx) {
    cmem_free(memory_tag_io_uring, ctx);
}