#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

// Error handling function to print error messages and exit
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Helper function to extract the file extension from a filename
char *get_file_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

// Function to send a file over a given socket descriptor
void send_file(int socketfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    char response_header[BUFFER_SIZE];
    FILE *file = fopen(filename, "rb"); // Open the file in read-binary mode

    if (!file) { // If the file does not exist, send a 404 Not Found
        snprintf(response_header, sizeof(response_header), "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n404 Not Found\n");
        send(socketfd, response_header, strlen(response_header), 0);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Determine content type based on file extension
    const char *ext = get_file_extension(filename);
    char *content_type = "text/plain"; // Default content type
    if (strcmp(ext, "html") == 0) content_type = "text/html";
    else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) content_type = "image/jpeg";
    else if (strcmp(ext, "gif") == 0) content_type = "image/gif";
    else if (strcmp(ext, "mp3") == 0) content_type = "audio/mpeg";
    else if (strcmp(ext, "pdf") == 0) content_type = "application/pdf";

    // Prepare and send the HTTP response header
    snprintf(response_header, sizeof(response_header), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n", file_size, content_type);
    send(socketfd, response_header, strlen(response_header), 0);

    // Send the file content in chunks
    while (!feof(file)) {
        int bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        send(socketfd, buffer, bytes_read, 0);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) { // Ensure the port number is passed as an argument
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sockfd < 0) error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr)); // Clear structure
    portno = atoi(argv[1]); // Convert port number from string to integer
    serv_addr.sin_family = AF_INET; // Address family
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Any incoming interface
    serv_addr.sin_port = htons(portno); // Convert port number to network byte order

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // Bind the address to the socket
        error("ERROR on binding");

    listen(sockfd, 5); // Listen on the socket for connections
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); // Accept a connection
        if (newsockfd < 0) error("ERROR on accept");

        bzero(buffer, BUFFER_SIZE); // Clear buffer
        if (read(newsockfd, buffer, BUFFER_SIZE - 1) < 0) error("ERROR reading from socket");

        // Parse the filename from the request
        char *filename = strchr(buffer, ' ');
        if (filename) {
            filename++; // Skip the space
            char *end = strchr(filename, ' ');
            if (end) *end = '\0'; // Null terminate the filename
        } else {
            filename = "index.html"; // Default file if no specific file is requested
        }

        send_file(newsockfd, filename); // Send the requested file
        close(newsockfd); // Close the client socket
    }

    close(sockfd); // Close the server socket
    return 0;
}
