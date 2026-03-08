#include "client_manager.h"

#include <core/memory/cmem.h>
#include <core/util/logger.h>

#include <sys/socket.h>
#include <unistd.h>

client_manager *client_manager_create() {
    client_manager* c_man = cmem_alloc(memory_tag_client_manager, sizeof(client_manager));

    const char* host = "127.0.0.1";
    int port = 8080;

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    c_man->addr.sin_family = AF_INET;
    c_man->addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &c_man->addr.sin_addr);
}

void client_manager_destroy(client_manager *c_man) {
    close(c_man->client_fd);
    cmem_free(memory_tag_client_manager, c_man);
    c_man = 0;
}

int client_manager_run(client_manager *c_man) {
    if (connect(c_man->client_fd, (struct sockaddr*)&c_man->addr, sizeof(c_man->addr)) == -1) {
        perror("connect");
        return -1;
    }

    char req[512];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n", path, host);

    send(fd, req, strlen(req), 0);

    char buf[4096];
    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
}