#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include "db/db.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int initialize_server_socket(const char *address, uint16_t port);

int accept_connection(int server_socket);

void start_server(const char *address, uint16_t port, const char *webroot);

int main(int argc, char *argv[]) {
    const char *address = "127.0.0.1"; // default addy
    uint16_t   port     = 8080;

    const char *webroot =
                       "webroot"; // Path to the directory where HTML files are stored
    if (argc > 1) {
        address = argv[1];
    }
    if (argc > 2) {
        // Use strtol for converting the port argument
        char *endptr;
        long parsed_port = strtol(argv[2], &endptr, 10);

        // Check for conversion errors
        if (*endptr != '\0' || parsed_port < 0 || parsed_port > UINT16_MAX) {
            fprintf(stderr, "Invalid port number: %s\n", argv[2]);
            return EXIT_FAILURE;
        }

        port = (uint16_t) parsed_port;
    }

    // Proceed to use the address and port for your server setup
    start_server(address, port, webroot);

    return 0;
}

//Initializes the server socket and inilization inside start_server
int initialize_server_socket(const char *address, uint16_t port) {
    int                server_socket;
    struct sockaddr_in server_addr;
    int                optval = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == -1) {
        perror("Setting socket options failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port        = htons(port);

    if (bind(server_socket, (struct sockaddr *) &server_addr,
             sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

int accept_connection(int server_socket) {
    struct sockaddr_in client_addr;
    socklen_t          client_len    = sizeof(client_addr);
    int                client_socket =
                               accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
    if (client_socket < 0) {
        perror("accept failed");
        return -1;
    }

    printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
    return client_socket;
}

noreturn void start_server(const char *address, uint16_t port,
                           const char *webroot) {
    int    server_socket               = initialize_server_socket(address, port);
    int    client_sockets[MAX_CLIENTS] = {0};
    int    max_sd, activity;
    fd_set read_fds;

    printf("Server listening on %s:%d\n", address, port);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &read_fds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        //Select call to make the array of file descriptors for IO multiplexing
        activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
            continue;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            int new_socket = accept_connection(server_socket);
            if (new_socket > 0) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &read_fds)) {
                char    buffer[BUFFER_SIZE];
                ssize_t valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread <= 0) {
                    // Handle connection closure or error
                    printf("Client disconnected or error occurred for client %d\n", i);

                    // Close the client socket
                    close(sd);

                    // Reset the client socket to 0
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Message from client %d: %s\n", i, buffer);


                    // GET starts here
                    if (strstr(buffer, "GET") != NULL) {
                        char method[16];
                        char path[1024];
                        char full_path[2048];
                        int  file_fd = -1;  // Initialize file_fd to an invalid value

                        sscanf(buffer, "%s %s", method, path);
                        printf("Received GET request for: %s\n", path);

                        // Check if the path is just "/"
                        if (strcmp(path, "/") == 0) {
                            // Redirect to index.html
                            snprintf(full_path, sizeof(full_path), "%s/index.html", webroot);
                            file_fd = open(full_path, O_RDONLY);
                        } else {
                            // Construct the full path to the requested file
                            snprintf(full_path, sizeof(full_path), "%s/%s", webroot, path);

                            // Open the requested file
                            file_fd = open(full_path, O_RDONLY);
                        }


                        // File not found, try to open 404.html
                        if (file_fd == -1) {
                            char not_found_path[2048];
                            snprintf(not_found_path, sizeof(not_found_path), "%s/404.html",
                                     webroot);
                            file_fd = open(not_found_path, O_RDONLY);



                            // 404.html found, send html with appropriate content
                            if (file_fd != -1) {
                                const char *html_content_type = "text/html";
                                char       file_buffer[BUFFER_SIZE];
                                ssize_t    bytes_read;

                                //Builds the response but sends it incrementally
                                send(sd, "HTTP/1.1 404 Not Found\r\n", 24,
                                     0); // Sending the HTTP status line
                                send(sd, "Content-Type: ", 14,
                                     0); // Sending the Content-Type header
                                send(sd, html_content_type, strlen(html_content_type), 0);
                                send(sd, "\r\n\r\n", 4,
                                     0); // End of headers, followed by content

                                //Reads file and puts stream into sd
                                while ((bytes_read = read(file_fd, file_buffer,
                                                          sizeof(file_buffer))) > 0) {
                                    // Cast bytes_read to size_t when passing to send
                                    send(sd, file_buffer, (size_t) bytes_read, 0);
                                }
                                close(file_fd);
                            } else {

                                // 404.html not found, send a default 404 response
                                const char not_found_response[] =
                                                   "HTTP/1.1 404 Not Found\r\n"
                                                   "Content-Type: text/html\r\n\r\n"
                                                   "<html>"
                                                   "<head><title>404 Not Found</title></head>"
                                                   "<body>"
                                                   "<h1>404 Not Found</h1>"
                                                   "<p>The requested page was not found.</p>"
                                                   "</body>"
                                                   "</html>";
                                send(sd, not_found_response, sizeof(not_found_response) - 1, 0);
                            }

                            // Requested file found, send its content with appropriate content type
                        } else {
                            const char *html_content_type = "text/html";
                            char       file_buffer[BUFFER_SIZE];
                            ssize_t    bytes_read;

                            //Sends the response header incrementally
                            send(sd, "HTTP/1.1 200 OK\r\n", 17,
                                 0); // Sending the HTTP status line
                            send(sd, "Content-Type: ", 14,
                                 0); // Sending the Content-Type header
                            send(sd, html_content_type, strlen(html_content_type), 0);
                            send(sd, "\r\n\r\n", 4, 0); // End of headers, followed by content

                            //Reads contents of file and puts it to sd for sending
                            while ((bytes_read = read(file_fd, file_buffer,
                                                      sizeof(file_buffer))) > 0) {
                                send(sd, file_buffer, (size_t) bytes_read, 0);
                            }
                            close(file_fd);
                        }


                        // POST starts here
                    } else if (strstr(buffer, "POST") != NULL) {
                        char dbName[] = "mydatabase.db";
                        char method[16];
                        char path[1024];
                        // Attempt to parse the method and path from the request
                        if (sscanf(buffer, "%s %s", method, path) == 2) {
                            printf("Parsed method: %s, Parsed path: %s\n", method, path);


                            // Inside the POST request handling block
                            if (strcmp(path, "/submit") == 0) {
                                char *bodyStart = strstr(buffer, "\r\n\r\n");
                                if (bodyStart) {
                                    size_t contentLength;
                                    // Calculate content length
                                    char   *contentLengthStr = strstr(buffer, "Content-Length: ");
                                    bodyStart += 4; // Move past the header/body separator to the start of the body
                                    contentLength            = 0;

                                    if (contentLengthStr) {
                                        sscanf(contentLengthStr, "Content-Length: %zu", &contentLength);
                                    }

                                    // Check content length and buffer size
                                    if (contentLength > 0 && contentLength < BUFFER_SIZE) {
                                        char *postData = (char *) malloc(contentLength + 1); // +1 for null terminator
                                        if (postData) {
                                            char *retrievedNote;
                                            strncpy(postData, bodyStart, contentLength);
                                            postData[contentLength] = '\0'; // Null-terminate the string
                                            // Open the database just for this operation
                                            openDatabase(dbName);

                                            // Process the POST data
                                            printf("POST Data: %s\n", postData);
                                            storeStringInDB(postData); // Store the data

                                            // Read the latest note from the database for the response
                                            retrievedNote = readStringFromDB();
                                            if (retrievedNote) {
                                                // Construct and send the response with the retrieved note
                                                char response[1024]; // Ensure this buffer is large enough
                                                int  responseLength = snprintf(response, sizeof(response),
                                                                               "HTTP/1.1 200 OK\r\n"
                                                                               "Content-Type: text/plain\r\n"
                                                                               "Content-Length: %zu\r\n\r\n"
                                                                               "This is read from DB:%s \n",
                                                                               strlen(retrievedNote) + 23,
                                                                               retrievedNote);
                                                if (responseLength > 0) {
                                                    send(sd, response, (size_t) responseLength, 0);
                                                }
                                            } else {
                                                printf("Failed to retrieve note from database.\n");
                                            }

                                            // Close the database after the operation
                                            closeDatabase();

                                            free(postData);
                                        } else {
                                            printf("Failed to allocate memory for POST data.\n");
                                        }
                                    } else {
                                        printf("Content-Length not found or exceeds buffer size.\n");
                                    }
                                } else {
                                    printf("POST request does not contain a body or header/body separator not found.\n");
                                }
                            }
                        } else {
                            printf("Failed to parse method and path from POST request.\n");
                        }


                        //HEAD starts here
                    } else if (strstr(buffer, "HEAD") != NULL) {
                        char method[16];
                        char path[1024];
                        // Attempt to parse the method and path from the request
                        if (sscanf(buffer, "%s %s", method, path) == 2) {
                            char        full_path[2048];
                            struct stat fileInfo;
                            printf("Parsed method: HEAD, Parsed path: %s\n", path);

                            // Determine the path to the resource as you would for a GET request
                            snprintf(full_path, sizeof(full_path), "%s/%s", webroot, path);

                            // Check if the file exists
                            if (stat(full_path, &fileInfo) == 0) {
                                // File exists, prepare the response headers
                                char header[1024];
                                snprintf(header, sizeof(header),
                                         "HTTP/1.1 200 OK\r\n"
                                         "Content-Length: %lld\r\n" // Use the actual file size
                                         "Content-Type: text/html; charset=UTF-8\r\n" // Assume HTML for simplicity
                                         "\r\n",
                                         (long long) fileInfo.st_size);

                                send(sd, header, strlen(header), 0);
                            } else {
                                // File not found, send 404 response
                                const char *response = "HTTP/1.1 404 Not Found\r\n"
                                                       "Content-Type: text/html\r\n"
                                                       "\r\n";
                                send(sd, response, strlen(response), 0);
                            }
                        }
                    }
                }
            }
        }
    }
}

