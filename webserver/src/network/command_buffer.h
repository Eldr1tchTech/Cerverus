#pragma once

#include "core/containers/queue.h"
#include "network/network_types.inl"

typedef enum command_type
{
    command_type_sendfile,
    command_type_sendresponse,
    command_type_close,
} command_type;

typedef struct command
{
    command_type cmd_type;
    union
    {
        struct
        {
            // TODO: Improve this, maybe take a file_fd and file_size.
            char *file_name;
        } sendfile_data;
        struct
        {
            response *res;
        } sendresponse_data;
    } data;
} command;

typedef struct command_buffer
{
    queue *cmd_queue;
} command_buffer;

command_buffer *command_buffer_create();
void command_buffer_destroy(command_buffer *cmd_buff);

void command_buffer_add(command_buffer *cmd_buff, command *cmd);
bool command_buffer_execute(command_buffer *cmd_buff, int client_fd);