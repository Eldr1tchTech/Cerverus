#pragma once

#include "server.h"
#include "core/memory/cmem.h"

#include <liburing.h>

#define BUFFER_SIZE 8192

typedef enum uring_op_type
{
    uring_op_type_accept,
    uring_op_type_recv,
    uring_op_type_openat,
    uring_op_type_send,
    uring_op_type_sendfile,
    uring_op_type_close,
} uring_op_type;

// This is what you store in set_data() - contains everything needed
// to resume when the operation completes
typedef struct uring_context
{
    uring_op_type op_type;
    server *srv;
    int file_fd;

    struct
    {
        struct sockaddr addr;
        socklen_t addrlen;
        int fd;
    } client;
    struct
    {
        char buffer[BUFFER_SIZE];
        size_t offset;
        request *request;
    } request;
    struct
    {
        response *response;
        char *buffer;
        size_t length;
        size_t offset;
    } response;
} uring_context;

typedef struct uring_close_context {
    uring_op_type op_type;
} uring_close_context;

void uring_process_completions();

void handle_accept_submission(server *srv);
void handle_accept_completion(struct io_uring_cqe *cqe, uring_context *ctx);

void handle_recv_submission(uring_context *ctx);
void handle_recv_completion(struct io_uring_cqe *cqe, uring_context *ctx);

void handle_openat_submission();
void handle_openat_completion();

void handle_send_submission();
void handle_send_completion();

void handle_sendfile_submission();
void handle_sendfile_completion();

void handle_close_submission();
void handle_close_completion();