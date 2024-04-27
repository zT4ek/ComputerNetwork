#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_CONTENT_SIZE 65536  // Define the maximum size of file content buffer(64KB)

// Custom error handling function that prints the error message and exits the application
void handle_error(char *msg)
{
    perror(msg);  // Use perror to give the error message with the context of the last error
    exit(EXIT_FAILURE);  // Exit the program with a failure response
}

int main(int argc, char *argv[])
{
    int server_fd, client_fd; // Descriptors for the server socket and the client socket
    unsigned int port_number; // Variable to store the port number on which the server will listen
    struct sockaddr_in server_addr, client_addr; // Structures to store address information for the server and client
    unsigned int client_addr_len; // Variable to store the length of the client address structure

    char request_buffer[1024]; // Buffer to store the HTTP request from the client

    // Check if the port number is provided as a command-line argument
    if (argc < 2)
        handle_error("ERROR: Port Number Not Provided");
    port_number = atoi(argv[1]); // Convert the command-line string argument to an integer

    // Create a socket for IPv4 with TCP communication
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        handle_error("ERROR: Failed to Open Socket");

    // Initialize server address structure
    server_addr.sin_family = AF_INET;  // Set the family of the address to IPv4
    server_addr.sin_port = htons(port_number);  // Convert the port number to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Allow the server to accept connections on any interface

    int reuse_addr = 1;  // Enable reusing the address immediately after the program terminates
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    // Bind the socket to the IP address and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        handle_error("ERROR: Failed to Bind");

    // Start listening on the server socket with a maximum of 5 pending connections
    if (listen(server_fd, 5) < 0)
        handle_error("ERROR: Failed to Listen");

    // Server main loop to accept client connections repeatedly
    while (1)
    {
        // Accept a new connection and create a new socket for this client
        client_addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&server_addr, &client_addr_len);
        if (client_fd < 0)
            handle_error("ERROR: Failed to Accept Connection");

        // Clear the request buffer to receive a new message
        memset(request_buffer, 0, sizeof(request_buffer));

        // Read the client's HTTP request into the buffer
        if (read(client_fd, request_buffer, 1023) < 0)
            handle_error("ERROR: Failed to Read from Socket");

        // Display the request for debugging purposes
        printf("=============================================\n");
        printf("Request: %s\n", request_buffer);

        char file_path[1024], content_type[128]; // Variables to hold the file path and content type
        // Attempt to parse the request assuming it's in the format "GET /filename HTTP/1.x"
        if (sscanf(request_buffer, "GET %s HTTP/1.", file_path) != 1)
            write(client_fd, "HTTP/1.1 400 Bad Request\n\n", 25);  // Send a bad request response if parsing fails
        else
        {
            // Check if the root is accessed and redirect to index.html
            if (strcmp(file_path, "/") == 0)
                strcpy(file_path, "/index.html");

            // Extract the file extension to determine the content type
            char *file_ext = strrchr(file_path, '.');
            if (file_ext) // Check if a file extension was found
            {
                // Determine the content type based on the file extension
                determine_content_type(file_ext, content_type);
            }
            else
                strcpy(content_type, "text/html"); // Default content type if no extension is found

            // Concatenate to form the complete path to the file
            char complete_path[1029] = ".";
            strcat(complete_path, file_path);
            printf("Path: %s\n", complete_path);
            printf("Content Type: %s\n", content_type);

            // Try to open the requested file
            int file_descriptor = open(complete_path, O_RDONLY);
            if (file_descriptor == -1)
                write(client_fd, "HTTP/1.1 404 Not Found\n\n", 24);  // Send a 404 response if the file is not found
            else
            {
                // Obtain file details like size
                struct stat file_stat;
                fstat(file_descriptor, &file_stat);
                char http_response_header[1024];
                snprintf(http_response_header, 1024, "HTTP/1.1 200 OK\nContent-Length: %ld\nContent-Type: %s\n\n", file_stat.st_size, content_type);
                write(client_fd, http_response_header, strlen(http_response_header)); // Send the HTTP response header

                // Send the file content in blocks
                char file_content[MAX_CONTENT_SIZE];
                ssize_t bytes_read;
                while ((bytes_read = read(file_descriptor, file_content, MAX_CONTENT_SIZE)) > 0)
                {
                    write(client_fd, file_content, bytes_read);
                }
                close(file_descriptor); // Close the file after sending
            }
        }
        close(client_fd); // Close the client connection
        printf("=============================================\n");
    }
    close(server_fd); // Close the server socket
    return 0;
}
