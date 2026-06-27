#include "io_uring_helper.h"

#include "network/http/request.h"
#include "core/util/logger.h"
#include "network/http/response.h"
#include "core/util/profiler.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void uring_process_completions(server *srv)
{
    struct io_uring_cqe *cqe;

    while (io_uring_peek_cqe(&srv->uring.ring, &cqe) == 0)
    {
        uring_context *ctx = (uring_context *)cqe->user_data;

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

        io_uring_cqe_seen(&srv->uring.ring, cqe);
    }
}

void handle_accept_submission(server *srv)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&srv->uring.ring);

    uring_context *ctx = pool_allocator_alloc(srv->uring.pool_alloc_ctx);
    if (!ctx) LOG_FATAL("handle_accept_submission - Requesting another ctx struct failed.");
    ctx->srv = srv;
    ctx->op_type = uring_op_type_accept;

    io_uring_prep_multishot_accept(sqe, srv->socket_fd, NULL, NULL, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&srv->uring.ring);
}

void handle_accept_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
    if (cqe->res < 0)
    {
        LOG_ERROR("handle_accept_completion - accept failed: %d", cqe->res);
    }
    else
    {
        uring_context *conn_ctx = cmem_alloc(memory_tag_io_uring, sizeof(uring_context));
        conn_ctx->srv = ctx->srv;
        conn_ctx->op_type = uring_op_type_recv;
        conn_ctx->client.fd = cqe->res;
        conn_ctx->request.offset = 0;
        handle_recv_submission(conn_ctx);
    }

    // The multishot op terminated — it must be re-armed.
    if (!(cqe->flags & IORING_CQE_F_MORE))
    {
        pool_allocator_free(ctx->srv->uring.pool_alloc_ctx, ctx);
        handle_accept_submission(ctx->srv);
    }
}

// TODO: If I were to pass ring as an argument, does it reduce lookup time, since it will never leave the registers?
void handle_recv_submission(uring_context *ctx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
    ctx->op_type = uring_op_type_recv;

    io_uring_prep_recv(sqe, ctx->client.fd, ctx->request.buffer + ctx->request.offset, BUFFER_SIZE - ctx->request.offset, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->uring.ring);
}

// TODO: Eventually should flush all requests within a read, not just the first one.
void handle_recv_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
    int bytes_read = cqe->res;
    if (bytes_read < 0)
    {
        LOG_ERROR("handle_recv_completion - Standard linux error.");
        return;
    } else if (bytes_read == 0)
    {
        LOG_DEBUG("handle_recv_completion - Client closed connection.");
        return;
    }

    int parse_result;
    do
    {
        parse_result = request_parse(ctx->request.buffer, bytes_read, &ctx->request.request);

        if (parse_result < 0)
        {
            // TODO: send error
            LOG_ERROR("handle_recv_completion - Error parsing, need to figure out how to flush the buffer safely.");
            return;
        }
        else if (parse_result == 0)
        {
            if (ctx->request.offset + bytes_read > BUFFER_SIZE)
            {
                LOG_ERROR("handle_recv_completion - Attempted to overflow buffer.");
            } else
            {
                ctx->request.offset += bytes_read;
            }
            
            handle_recv_submission(ctx);
        }
        else
        {
            cmem_mcpy(ctx->request.buffer, ctx->request.buffer + parse_result, ctx->request.offset - parse_result);
            ctx->request.offset -= parse_result;
            
            profile_operation("router_handle_request",
                router_handle_request(ctx->srv->rtr, &ctx->request.request, ctx->client.fd);
            );
            
        }
    } while (parse_result != 0); // TODO: Figure out how to flush all requests from the buffer and attempt to parse them.
}

// TODO: Eventually implement some sort of LRU_cache
void handle_openat_submission(uring_context *ctx, char *path)
{
    

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
    ctx->op_type = uring_op_type_openat;

    io_uring_prep_openat(sqe, 0, path, 0, O_RDONLY);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->uring.ring);
}

void handle_openat_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
    if (cqe->res <= 0)
    {
        LOG_ERROR("handle_openat_completion - Failed.");
        return;
    }

    ctx->file_fd = cqe->res;
}

void handle_send_submission(uring_context *ctx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
    ctx->op_type = uring_op_type_send;

    if (!ctx->response.buffer)
    {
        ctx->response.buffer = response_serialize(ctx->response.response); // TODO: Eventually have this fill the length.
        ctx->response.length = strlen(ctx->response.buffer);
        ctx->response.offset = 0;
    }

    io_uring_prep_send(sqe, ctx->client.fd, ctx->response.buffer + (sizeof(char) * ctx->response.offset), ctx->response.length, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->uring.ring);
}

void handle_send_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
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

    handle_close_submission(&ctx->srv->uring.ring, ctx->client.fd); // TODO: Please note that you need to take care of file desccriptors here
    // The best solution is probably to wrap the openat with an check to an internal file_fd cache.
}

void handle_sendfile_submission(uring_context *ctx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->srv->uring.ring);
    ctx->op_type = uring_op_type_sendfile;

    io_uring_prep_splice(sqe, ctx->file_fd, 0, ctx->client.fd, 0, 0, 0);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(&ctx->srv->uring.ring);
}

void handle_sendfile_completion(struct io_uring_cqe *cqe, uring_context *ctx)
{
}

void handle_close_submission(struct io_uring *ring, int fd)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    uring_close_context *ctx = cmem_alloc(memory_tag_io_uring, sizeof(uring_close_context));
    ctx->op_type = uring_op_type_close;

    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, ctx);

    io_uring_submit(ring);
}

void handle_close_completion(uring_context *ctx)
{
    cmem_free(memory_tag_io_uring, ctx);
}