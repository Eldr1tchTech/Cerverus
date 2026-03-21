#pragma once

#include <liburing.h>

typedef struct io_context {
    int client_fd;

    char buffer[8192];
    int offset;
} io_context;

void io_uring_handle_accept();

void io_uring_handle_read();