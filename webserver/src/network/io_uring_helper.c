#include "io_uring_helper.h"

#include "network/request.h"

#include <netinet/in.h>

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

    io_uring_prep_accept(sqe, srv->socket_fd, &ctx->client_addr, &ctx->client_addrlen, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&srv->ring);
}

void handle_accept_completion(struct io_uring_cqe* cqe, uring_context* ctx)
{
    if (cqe->res < 0)
    {
        cmem_free(memory_tag_io_uring, ctx);
        handle_accept_submission(ctx->srv);
        return;
    }

    // Logging code
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *addr = (struct sockaddr_in *)&ctx->client_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    int port = ntohs(addr->sin_port);

    LOG_INFO("Connection from: %s:%d", ip, port);

    handle_accept_submission(ctx->srv);

    ctx->client_fd = cqe->res;
    handle_recv_submission(ctx);
}

// TODO: If I were to pass ring as an argument, does it reduce lookup time, since it will never leave the registers?
void handle_recv_submission(uring_context* ctx) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->ring);
    ctx->op_type = uring_op_type_recv;

    io_uring_prep_recv(sqe, ctx->client_fd, ctx->buffer, BUFFER_SIZE, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->ring);
}

void handle_recv_completion(struct io_uring_cqe* cqe, uring_context* ctx) {
    request_parse(ctx->buffer, BUFFER_SIZE, ctx->req);
}

void handle_openat_submission();
void handle_openat_completion();

void handle_send_submission();
void handle_send_completion();

void handle_sendfile_submission();
void handle_sendfile_completion();

void handle_close_submission();
void handle_close_completion();