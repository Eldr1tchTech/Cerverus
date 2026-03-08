#pragma once

#include <core/containers/darray.h>

#include <arpa/inet.h>

typedef struct client_manager
{
    int client_fd;
    char** URIs;
    struct sockaddr_in addr;
} client_manager;

typedef enum benchmark_version {
    benchmark_version_0p1p0,
} benchmark_version;

typedef struct connection_benchmark_result
{
    double duration;
    char* URI;
    int status_code;
} connection_benchmark_result;

typedef struct benchmark_result
{
    benchmark_version bm_v;
    double benchmark_duration;
    darray* conn_bm_result;
} benchmark_result;

client_manager *client_manager_create();
void client_manager_destroy(client_manager *c_man);

int client_manager_run(client_manager *c_man);