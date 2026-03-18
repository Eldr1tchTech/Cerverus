#define _GNU_SOURCE
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define PORT        8080
#define QUEUE_DEPTH 8
#define BUF_SIZE    1024

typedef enum op_type {
    OP_ACCEPT,
    OP_RECV,
    OP_SEND,
} op_type;

typedef struct io_context {
    op_type type;
    int     client_fd;
    char    buf[BUF_SIZE];
} io_context;

// With SQPOLL the kernel thread may have gone idle. io_uring_submit() wakes
// it if needed, and is a cheap no-op if it's already running.
// Wrapping it makes the intent clear at each call site.
static void submit(struct io_uring *ring) {
    int ret = io_uring_submit(ring);
    if (ret < 0 && ret != -EBUSY) {
        fprintf(stderr, "submit failed: %s\n", strerror(-ret));
    }
}

static void submit_accept(struct io_uring *ring, int server_fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_context *ctx = malloc(sizeof(io_context));
    ctx->type      = OP_ACCEPT;
    ctx->client_fd = -1;
    io_uring_prep_accept(sqe, server_fd, NULL, NULL, 0);
    io_uring_sqe_set_data(sqe, ctx);
}

static void submit_recv(struct io_uring *ring, int client_fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_context *ctx = malloc(sizeof(io_context));
    ctx->type      = OP_RECV;
    ctx->client_fd = client_fd;
    memset(ctx->buf, 0, BUF_SIZE);
    io_uring_prep_recv(sqe, client_fd, ctx->buf, BUF_SIZE, 0);
    io_uring_sqe_set_data(sqe, ctx);
}

static void submit_send(struct io_uring *ring, int client_fd, char *data, int len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_context *ctx = malloc(sizeof(io_context));
    ctx->type      = OP_SEND;
    ctx->client_fd = client_fd;
    memcpy(ctx->buf, data, len);
    io_uring_prep_send(sqe, client_fd, ctx->buf, len, MSG_NOSIGNAL);
    io_uring_sqe_set_data(sqe, ctx);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("Echo server listening on port %d\n", PORT);

    // --- Only this block changed from the previous example ---
    struct io_uring ring;
    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    params.flags          |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle  = 2000; // kernel thread sleeps after 2s of inactivity

    int ret = io_uring_queue_init_params(QUEUE_DEPTH, &ring, &params);
    if (ret < 0) {
        fprintf(stderr, "Failed to init io_uring: %s\n", strerror(-ret));
        return 1;
    }
    // ---------------------------------------------------------

    submit_accept(&ring, server_fd);
    submit(&ring); // wake/notify kernel thread of the first accept

    while (1) {
        struct io_uring_cqe *cqe;

        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            fprintf(stderr, "wait_cqe: %s\n", strerror(-ret));
            break;
        }

        io_context *ctx = io_uring_cqe_get_data(cqe);

        switch (ctx->type) {
            case OP_ACCEPT:
                if (cqe->res < 0) {
                    fprintf(stderr, "accept failed: %s\n", strerror(-cqe->res));
                } else {
                    int client_fd = cqe->res;
                    printf("Client connected: fd=%d\n", client_fd);
                    submit_recv(&ring, client_fd);
                    submit_accept(&ring, server_fd);
                    submit(&ring);
                }
                free(ctx);
                break;

            case OP_RECV:
                if (cqe->res <= 0) {
                    printf("Client fd=%d disconnected.\n", ctx->client_fd);
                    close(ctx->client_fd);
                } else {
                    printf("Received %d bytes: %.*s", cqe->res, cqe->res, ctx->buf);
                    submit_send(&ring, ctx->client_fd, ctx->buf, cqe->res);
                    submit(&ring);
                }
                free(ctx);
                break;

            case OP_SEND:
                if (cqe->res < 0) {
                    fprintf(stderr, "send failed: %s\n", strerror(-cqe->res));
                    close(ctx->client_fd);
                } else {
                    submit_recv(&ring, ctx->client_fd);
                    submit(&ring);
                }
                free(ctx);
                break;
        }

        io_uring_cqe_seen(&ring, cqe);
    }

    close(server_fd);
    io_uring_queue_exit(&ring);
    return 0;
}