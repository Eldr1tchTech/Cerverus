#include "server.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "network/router.h"
#include "network/request.h"
#include "network/response.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

void server_destroy(server* s) {

}

server* server_create(server_config s_conf) {
    server* s = cmem_alloc(memory_tag_server, sizeof(server));

    s->port = s_conf.port;

    return s;
}

void server_run(server* s) {
    if (!s->r)
    {
        LOG_ERROR("server_start - The server must first have a router.");
        return;
    }

    LOG_INFO("Starting server...");
    
    s->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->socket_fd == -1)
    {
        LOG_FATAL("server_start - Socket creation failed.");
        return;
    }

    int opt = 1;
    if (setsockopt(s->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_FATAL("server_start - setsockopt failed.");
        close(s->socket_fd);
        return;
    }

    // Enable non-blocking mode (optional)
    // fcntl(server_fd, F_SETFL, O_NONBLOCK);
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(s->port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(s->socket_fd, &addr, sizeof(addr)) == -1)
    {
        LOG_FATAL("server_start - Bind failed.");
        return;
    }
    
    if (listen(s->socket_fd, 10) == -1)
    {
        LOG_FATAL("server_start - Listen failed.");
        return;
    }
    
    LOG_INFO("Server listening on port %i.", s->port);

    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(s->socket_fd, 0, 0);
        if (client_fd == -1)
        {
            LOG_INFO("server_start - accept failed.");
            continue;
        }

        char buffer[8192] = {0};
        ssize_t bytes_received = recv(client_fd, buffer, 8192, 0);

        if (bytes_received <= 0)
        {
            if (bytes_received == -1)
            {
                LOG_INFO("server_start - recv failed.");
            } else {
                LOG_INFO("Client disconnected.");
            }
            close(client_fd);
            continue;
        }

        request* req = request_parse(buffer);
        response* res = cmem_alloc(memory_tag_response, sizeof(response));

        LOG_INFO("Request received.");

        router_handle_route(s->r, req, res);

        cmem_free(memory_tag_response, res->body.data);
        cmem_free(memory_tag_response, res->headers.headers);
        cmem_free(memory_tag_response, res);

        send(client_fd, response_serialize(res), 1892, 0);
        
        close(client_fd);
    }
    
    close(s->socket_fd);
}