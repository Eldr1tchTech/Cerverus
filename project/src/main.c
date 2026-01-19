#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) { perror("Socket failed"); return 1; }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Bind failed");
        return 1;
    }

    if (listen(s, 10) == -1) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port 8080...\n");

    while (1) {
        int client_fd = accept(s, 0, 0);
        if (client_fd == -1) continue;

        char buffer[256] = {0};
        recv(client_fd, buffer, 256, 0);

        // buffer contains: "GET /index.html HTTP/1.1..."
        
        // Safety check: ensure buffer is long enough
        if (strlen(buffer) < 5) {
            close(client_fd);
            continue;
        }

        printf("Received request: \n%s", buffer);

        // 1. Skip "GET /" (5 characters)
        char* f = buffer + 5; 

        // 2. Find the space AFTER the filename
        char* end = strchr(f, ' ');
        if (end) {
            *end = 0; // Terminate the string here
        }

        printf("Requested file: %s\n", f);

        // Simple security check to prevent directory traversal
        if (strstr(f, "..")) {
             close(client_fd);
             continue;
        }

        char file_name[256] = {0};
        sprintf(file_name, "assets/%s", f);

        int opened_fd = open(file_name, O_RDONLY);
        
        if (opened_fd != -1) {
            // 1. Get the file size using fstat
            struct stat file_stat;
            fstat(opened_fd, &file_stat);
            
            // 2. Format the headers properly with Content-Length
            // We use a larger buffer to be safe
            char response_buffer[512] = {0};
            
            // Note: %ld is for long int (off_t is usually long)
            sprintf(response_buffer, 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %ld\r\n"
                    "\r\n", 
                    file_stat.st_size);
            
            // 3. Send headers
            send(client_fd, response_buffer, strlen(response_buffer), 0);
            
            // 4. Send the EXACT amount of file data
            // (Using the file size instead of hardcoded 1024)
            off_t offset = 0;
            sendfile(client_fd, opened_fd, &offset, file_stat.st_size);
            
            close(opened_fd);
        } else {
            // Optional: Send 404 if file not found
            char *not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(client_fd, not_found, strlen(not_found), 0);
            perror("File open failed");
        }
        
        close(client_fd);
    }

    close(s);
    return 0;
}