#include "server.h"

#include "defines.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "core/util/profiler.h"
#include "core/util/util.h"
#include "network/request.h"
#include "network/response.h"
#include "network/route_trie.h"
#include "network/network_util.h"
#include "network/io_uring_helper.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#define QUEUE_DEPTH 32

server *server_create(server_config* s_conf)
{
    server *s = cmem_alloc(memory_tag_server, sizeof(server));
    s->conf = s_conf;
    s->route_trie = trie_create();

    return s;
}

void server_add_route(server *s, route *rt)
{
    trie_add_route(s->route_trie, rt);
}

void send_file_response(int client_fd, int file_fd, int status_code, const char *reason_phrase, char *ext)
{
    // 1. Assemble response
    // TODO: Eventually use a pool for this
    response *res = response_create(0);

    res->status_line.version = http_version_1p1;
    res->status_line.status_code = 200;
    res->status_line.reason_phrase = "OK";

    struct stat file_stat;
    fstat(file_fd, &file_stat);

    // Headers
    response_add_header(res, (header){.name = "Content-Type", .value = content_type_val_helper(ext)});
    char *content_length_str = asprintf("%i", file_stat.st_size);
    response_add_header(res, (header){.name = "Content-Length", .value = content_length_str});
    response_add_header(res, (header){.name = "Connection", .value = "close"});

    // 2. Send response and file
    char *raw = response_serialize(res);
    send(client_fd, raw, strlen(raw), MSG_NOSIGNAL);
    sendfile(client_fd, file_fd, 0, file_stat.st_size);
    cmem_free(memory_tag_response, raw);
    cmem_free(memory_tag_string, content_length_str);   // Find some way to get rid of this...
    /* IDEA:
    Allocate a buffer that should be big enough, use snprintf, if it fails, allocate enough
    */

    close(file_fd);
}

void server_handle_request(server *s, request *req, int client_fd)
{
    if (strcmp(req->request_line.URI, "/") == 0)
    {
        int file_fd = open("assets/public/index.html", O_RDONLY);
        if (file_fd != -1)
        {
            send_file_response(client_fd, file_fd, 200, "OK", ".html");
            return;
        }
    }

    // 1. Check public directory
    if (req->request_line.method == http_method_get)
    {
        const char *ext = strrchr(req->request_line.URI, '.');
        if (ext)
        {
            char* file_name = asprintf("assets/public%s", req->request_line.URI);
            int file_fd = open(file_name, O_RDONLY);
            if (file_fd != -1)
            {
                send_file_response(client_fd, file_fd, 200, "OK", ext);
                cmem_free(memory_tag_string, file_name);
                return;
            }
            cmem_free(memory_tag_string, file_name);
        }
    }

    // 2. Check against dynamic registered routes
    route_callback handler = trie_find_handler(s->route_trie, req->request_line.method, req->request_line.URI);
    if (handler)
    {
        (*handler)(req, client_fd);
        return;
    }

    // 3. Send 404 if you have made it to this point
    int file_fd = open("assets/404.html", O_RDONLY);
    if (file_fd != -1)
    {
        send_file_response(client_fd, file_fd, 404, "Not Found", ".html");
    }
}

void server_destroy(server *s)
{
    cmem_print_stats();

    trie_destroy(s->route_trie);
    cmem_free(memory_tag_server, s);
}

void server_run(server *s)
{
    // Install SIGPIPE handler to prevent crashes
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // No SA_RESETHAND — disposition stays ignored permanently
    sigaction(SIGPIPE, &sa, NULL);

    cmem_print_stats();

    LOG_INFO("Starting server...");
    LOG_INFO("Setting up socket...");

    s->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->socket_fd == -1)
    {
        LOG_FATAL("server_start - Socket creation failed.");
        return;
    }

    int opt = 1;
    if (setsockopt(s->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        LOG_FATAL("server_start - setsockopt failed.");
        close(s->socket_fd);
        return;
    }

    const struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY};

    if (bind(s->socket_fd, &addr, sizeof(addr)) == -1)
    {
        LOG_FATAL("server_start - Bind failed.");
        close(s->socket_fd);
        return;
    }

    socklen_t len = sizeof(addr);
    getsockname(s->socket_fd, (struct sockaddr *)&addr, &len);

    if (listen(s->socket_fd, 10) == -1)
    {
        LOG_FATAL("server_start - Listen failed.");
        close(s->socket_fd);
        return;
    }

    LOG_INFO("Setting up io_uring...");

    struct io_uring_params params;
    cmem_zmem(&params, sizeof(params));
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 2000; // 2s timeout

    int ret = io_uring_queue_init_params(QUEUE_DEPTH, &s->ring, &params);
    if (ret < 0)
    {
        LOG_FATAL("server_run - io_uring init failed.");
        close(s->socket_fd);
        return;
    }

    // TODO: Submit accepts here

    LOG_INFO("Server listening on port %i.\n\tVisit: http://localhost:%i/index.html", ntohs(addr.sin_port), ntohs(addr.sin_port));

    while (true)
    {
        process_completions();
    }

    close(s->socket_fd);
    io_uring_queue_exit(&s->ring);

    server_destroy(s);
}