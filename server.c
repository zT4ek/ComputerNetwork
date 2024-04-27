#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CONTENT_SIZE 65536

// When an Error Occurs
void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int socketfd, new_socketfd; // Socket File Descriptor
    unsigned int port_number;   // Port Number
    struct sockaddr_in server_address, client_address;
    unsigned int client_length;

    char buffer[1024];

    if (argc < 2)
        error("ERROR Port Number is Not Provided");
    port_number = atoi(argv[1]);

    /*
     * Create Socket
     * IPv4, TCP, auto protocol
     */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
        error("ERROR Open Socket");

    server_address.sin_family = AF_INET;          // Set IPv4
    server_address.sin_port = htons(port_number); // convert from host to network byte order
    server_address.sin_addr.s_addr = INADDR_ANY;  // Set the server socket to accept incoming connection requests from all network interfaces

    int optival = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optival, sizeof(optival)); // Port Reuse Faster

    // Bind the Socket to the Server Address
    if (bind(socketfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        error("ERROR Binding");
    // Listen for Socket Connections. Backlog Queue Size is 5
    if (listen(socketfd, 5) < 0)
        error("ERROR Listen");

    while (1)
    {
        // Accept Client Link
        client_length = sizeof(client_address);
        new_socketfd = accept(socketfd, (struct sockaddr *)&server_address, &client_length);
        if (new_socketfd < 0)
            error("ERROR Accept");

        memset(buffer, 0, sizeof(buffer)); // Reset Buffer

        // Read Request from Client
        if (read(new_socketfd, buffer, 1023) < 0)
            error("ERROR Reading from Socket");

        // Check Request
        printf("=============================================\n");
        printf("%s\n", buffer);

        char path[1024], file_type[126];
        if (sscanf(buffer, "GET %s HTTP/1.", path) != 1)
            write(new_socketfd, "HTTP/1.1 400 Bad Request\n\n", 29);
        else
        {
            // If Client Access Root, Automatically provides index.html
            if (strcmp(path, "/") == 0)
                strcpy(path, "/index.html");

            char *file_name = path;
            char *file_extension = strrchr(file_name, '.'); // find the last dot in file_name
            if (file_extension)
            {
                if (strcmp(file_extension, ".html") == 0)
                    strcpy(file_type, "text/html");
                else if (strcmp(file_extension, ".gif") == 0)
                    strcpy(file_type, "image/gif");
                else if (strcmp(file_extension, ".jpeg") == 0 || strcmp(file_extension, ".jpg") == 0)
                    strcpy(file_type, "image/jpeg");
                else if (strcmp(file_extension, ".png") == 0)
                    strcpy(file_type, "image/png");
                else if (strcmp(file_extension, ".mp3") == 0)
                    strcpy(file_type, "audio/mpeg");
                else if (strcmp(file_extension, ".pdf") == 0)
                    strcpy(file_type, "application/pdf");
                else if (strcmp(file_extension, ".ico") == 0)
                    strcpy(file_type, "image/x-icon");
                else
                    strcpy(file_type, "text/plain");
            }
            else
                strcpy(file_type, "text/html");

            char file_path[1024 + 5] = ".";
            strcat(file_path, file_name);
            printf("file path: %s\n", file_path);
            printf("file type: %s\n", file_type);

            // Open File
            int file_fd = open(file_path, O_RDONLY);
            if (file_fd == -1)
                write(new_socketfd, "HTTP/1.1 404 Not Found\n\n", 29);
            else
            {
                // Get File Size
                struct stat file_stat;
                fstat(file_fd, &file_stat);
                char http_msg[1024];
                snprintf(http_msg, 1024, "HTTP/1.1 200 OK\nContent-Length: %ld\nContent-Type: %s\n\n", file_stat.st_size, file_type);
                write(new_socketfd, http_msg, strlen(http_msg));

                char content[CONTENT_SIZE];
                ssize_t read_len;
                while ((read_len = read(file_fd, content, CONTENT_SIZE)) > 0)
                {
                    write(new_socketfd, content, read_len);
                }
                close(file_fd);
            }
        }
        close(new_socketfd);
        printf("=============================================\n");
    }
    close(socketfd);
    return 0;
}
