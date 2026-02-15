#include "command_buffer.h"

#include "core/memory/cmem.h"
#include "core/util/logger.h"
#include "network/response.h"

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

command_buffer *command_buffer_create()
{
    command_buffer *cmd_buff = cmem_alloc(memory_tag_command_buffer, sizeof(command_buffer));
    cmd_buff->cmd_queue = queue_create(sizeof(command));
}

void command_buffer_destroy(command_buffer *cmd_buff)
{
    queue_destroy(cmd_buff->cmd_queue);
    cmem_free(memory_tag_command_buffer, cmd_buff);
    cmd_buff = 0;
}

void command_buffer_add(command_buffer *cmd_buff, command *cmd)
{
    enqueue(cmd_buff->cmd_queue, cmd);
}

// Incorrect memory tag usage
// TODO: Update the memory system. Does not have to be fully dynamic and custom, but somewhat at least.
bool command_buffer_execute(command_buffer *cmd_buff, int client_fd)
{
    bool success = true;
    command *cmd;
    while (cmd_buff->cmd_queue->root)
    {
        cmd = dequeue(cmd_buff->cmd_queue);

        switch (cmd->cmd_type)
        {
        case command_type_sendresponse:
            // TODO: The 1982 needs to be removed and replaced with proper length handling and such.
            send(client_fd, response_serialize(cmd->data.sendresponse_data.res), 1982, 0);
            break;
        // NOTE: This needs to be reviewed once windows support is added.
        case command_type_sendfile:
            int file_fd = open(cmd->data.sendfile_data.file_name, O_RDONLY);
            if (file_fd == -1)
            {
                success = false;
                break;
            }

            struct stat file_stat;
            fstat(file_fd, &file_stat);

            sendfile(client_fd, file_fd, 0, file_stat.st_size);

            close(file_fd);
            break;
        case command_type_close:
            close(client_fd);
            break;
        default:
            LOG_ERROR("command_buffer_execute - Unsupported command_type.");
            break;
        }
        cmem_free(memory_tag_queue, cmd);
    }
    return success;
}