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

                    // Check if it's a GET request
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
                        if (file_fd == -1) {
                            // File not found, try to open 404.html
                            char not_found_path[2048];
                            snprintf(not_found_path, sizeof(not_found_path), "%s/404.html",
                                     webroot);
                            file_fd = open(not_found_path, O_RDONLY);

                            if (file_fd != -1) {
                                // 404.html found, send its content with appropriate content
                                // type

                                const char *html_content_type = "text/html";
                                char       file_buffer[BUFFER_SIZE];
                                ssize_t    bytes_read;
                                send(sd, "HTTP/1.1 404 Not Found\r\n", 24,
                                     0); // Sending the HTTP status line
                                send(sd, "Content-Type: ", 14,
                                     0); // Sending the Content-Type header
                                send(sd, html_content_type, strlen(html_content_type), 0);
                                send(sd, "\r\n\r\n", 4,
                                     0); // End of headers, followed by content

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
                        } else {
                            // File found, send its content with appropriate content type
                            const char *html_content_type = "text/html";
                            char       file_buffer[BUFFER_SIZE];
                            ssize_t    bytes_read;
                            send(sd, "HTTP/1.1 200 OK\r\n", 17,
                                 0); // Sending the HTTP status line
                            send(sd, "Content-Type: ", 14,
                                 0); // Sending the Content-Type header
                            send(sd, html_content_type, strlen(html_content_type), 0);
                            send(sd, "\r\n\r\n", 4, 0); // End of headers, followed by content

                            while ((bytes_read = read(file_fd, file_buffer,
                                                      sizeof(file_buffer))) > 0) {
                                // Cast bytes_read to size_t when passing to send
                                send(sd, file_buffer, (size_t) bytes_read, 0);
                            }
                            close(file_fd);
                        }
                    } else if (strstr(buffer, "POST") != NULL) {
                        // Handle POST request
                        char method[16];
                        char path[1024];
                        sscanf(buffer, "%s %s", method, path);
                        printf("Received POST request for: %s\n", path);

                        // Check if it's a POST request to a specific URL, e.g., /submit
                        if (strcmp(path, "/submit") == 0) {
                            // Parse the headers to determine the content length
                            int  content_length         = 0;
                            char *content_length_header = strstr(buffer, "Content-Length:");
                            if (content_length_header) {
                                sscanf(content_length_header, "Content-Length: %d",
                                       &content_length);
                            }

                            if (content_length > 0) {
                                // Read the request body based on the content length
                                char *post_data = (char *) malloc((size_t) (content_length + 1));
                                if (post_data) {
                                    ssize_t bytes_read =
                                                    read(sd, post_data, (size_t) content_length);
                                    if (bytes_read == content_length) {

                                        // Successfully received the POST data
                                        post_data[content_length] = '\0';
                                        printf("POST Data: %s\n", post_data);


                                    } else {
                                        // Error reading POST data
                                        const char *response =
                                                           "HTTP/1.1 400 Bad Request\r\n"
                                                           "Content-Type: text/html\r\n\r\n"
                                                           "<html><body><h1>Bad Request</h1></body></html>";
                                        send(sd, response, strlen(response), 0);
                                    }

                                    free(post_data); // Free the dynamically allocated memory
                                }
                            }
                        }
                    } else {
                        // Handle other POST requests or URLs
                        // You can add logic for different POST requests here
                    }
                }

                // Additional logic to handle the request can be added here
            }
        }
    }
}
