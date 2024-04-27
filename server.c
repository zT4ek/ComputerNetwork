#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 65536  // Maximum size for file read buffer(64KB)

// Function to handle errors: prints the error message and exits the application
void handleError(char *errorMessage)
{
    perror(errorMessage);  // Print the system error message
    exit(EXIT_FAILURE);  // Exit with a failure status
}

int main(int argc, char *argv[])
{
    int serverSocket, clientSocket;  // Descriptors for server and client sockets
    struct sockaddr_in serverAddr, clientAddr;  // Structures to store IP address and port for server and client
    socklen_t clientAddrLen;  // Length of client address data structure
    unsigned int serverPort;  // Port number for the server to listen on
    char requestBuffer[1024];  // Buffer to store incoming client requests

    // Check if the server port number is provided as a command-line argument
    if (argc < 2)
        handleError("ERROR: Port number not provided");  // Exit if no port provided
    serverPort = atoi(argv[1]);  // Convert the port number from string to integer

    // Create a TCP socket for IPv4 communication
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
        handleError("ERROR: Unable to open socket");  // Check for socket creation failure

    // Set up the server address structure
    serverAddr.sin_family = AF_INET;  // Address family for IPv4
    serverAddr.sin_port = htons(serverPort);  // Convert the port number to network byte order
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all network interfaces

    // Enable port reuse to avoid "address already in use" errors
    int optValue = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optValue, sizeof(optValue));

    // Bind the server socket to the specified IP address and port
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        handleError("ERROR: Binding failed");  // Check for binding error

    // Start listening on the server socket, with a maximum of 5 pending connections
    if (listen(serverSocket, 5) < 0)
        handleError("ERROR: Listening failed");  // Check for listening error

    // Server main loop to handle incoming connections
    while (1)
    {
        clientAddrLen = sizeof(clientAddr);
        // Accept a connection request from a client
        clientSocket = accept(serverSocket, (struct sockaddr *)&serverAddr, &clientAddrLen);
        if (clientSocket < 0)
            handleError("ERROR: Accepting connection failed");  // Check for accepting error

        memset(requestBuffer, 0, sizeof(requestBuffer));  // Clear the buffer before reading

        // Read the HTTP request from the client into the buffer
        if (read(clientSocket, requestBuffer, 1023) < 0)
            handleError("ERROR: Reading from socket failed");  // Check for reading error

        // Output the HTTP request for debugging
        printf("=============================================\n");
        printf("Request: %s\n", requestBuffer);

        char filePath[1024], contentType[128];  // Variables to hold the requested file path and content type
        // Parse the HTTP GET request to extract the file path
        if (sscanf(requestBuffer, "GET %s HTTP/1.", filePath) != 1)
            write(clientSocket, "HTTP/1.1 400 Bad Request\n\n", 26);  // Handle bad request
        else
        {
            // Serve index.html if the root directory is accessed
            if (strcmp(filePath, "/") == 0)
                strcpy(filePath, "/index.html");

            // Determine the content type based on the file extension
            char *fileExt = strrchr(filePath, '.');
            if (fileExt)
            {
                // Map file extension to MIME types
                if (strcmp(fileExt, ".html") == 0) strcpy(contentType, "text/html");
                else if (strcmp(fileExt, ".gif") == 0) strcpy(contentType, "image/gif");
                else if (strcmp(fileExt, ".jpeg") == 0 || strcmp(fileExt, ".jpg") == 0) strcpy(contentType, "image/jpeg");
                else if (strcmp(fileExt, ".png") == 0) strcpy(contentType, "image/png");
                else if (strcmp(fileExt, ".mp3") == 0) strcpy(contentType, "audio/mpeg");
                else if (strcmp(fileExt, ".pdf") == 0) strcpy(contentType, "application/pdf");
                else if (strcmp(fileExt, ".ico") == 0) strcpy(contentType, "image/x-icon");
                else strcpy(contentType, "text/plain");
            }
            else {
                strcpy(contentType, "text/html");  // Default content type
            }

            // Print the path and content type for debugging
            printf("Path: %s\n", filePath);
            printf("Content-Type: %s\n", contentType);

            // Construct the full file path
            char fullFilePath[1029] = ".";
            strcat(fullFilePath, filePath);

            // Attempt to open the requested file
            int fileDesc = open(fullFilePath, O_RDONLY);
            if (fileDesc == -1) {
                write(clientSocket, "HTTP/1.1 404 Not Found\n\n", 24);  // Send a 404 Not Found error if file is missing
            } else {
                struct stat fileInfo;
                fstat(fileDesc, &fileInfo);  // Get file size
                char httpResponse[1024];
                // Create the HTTP response header
                snprintf(httpResponse, sizeof(httpResponse), "HTTP/1.1 200 OK\nContent-Length: %ld\nContent-Type: %s\n\n", fileInfo.st_size, contentType);
                write(clientSocket, httpResponse, strlen(httpResponse));  // Send the HTTP response

                char fileContent[MAX_BUFFER_SIZE];
                ssize_t bytesRead;
                // Read and send the file content in chunks
                while ((bytesRead = read(fileDesc, fileContent, MAX_BUFFER_SIZE)) > 0)
                    write(clientSocket, fileContent, bytesRead);
                close(fileDesc);  // Close the file after sending
            }
        }
        close(clientSocket);  // Close the client socket
        printf("=============================================\n");
    }
    close(serverSocket);  // Shut down the server
    return 0;
}
