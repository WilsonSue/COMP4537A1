#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdnoreturn.h>
#include <stdint.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int initialize_server_socket(const char *address, uint16_t port);
int accept_connection(int server_socket);
void start_server(const char *address, uint16_t port);
int main(int argc, char *argv[]) {
    const char *address = "127.0.0.1";
    uint16_t port = 8080;

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

        port = (uint16_t)parsed_port;
    }

    // Proceed to use the address and port for your server setup
    start_server(address, port);

    return 0;
}

int initialize_server_socket(const char *address, uint16_t port) {
    int server_socket;
    struct sockaddr_in server_addr;
    int optval = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("Setting socket options failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
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
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("accept failed");
        return -1;
    }

    printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return client_socket;
}
noreturn void start_server(const char *address, uint16_t port) {
    int server_socket = initialize_server_socket(address, port);
    int client_sockets[MAX_CLIENTS] = {0};
    int max_sd, activity;
    fd_set read_fds;

    // Declare these variables at the beginning of your function
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

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
                char buffer[BUFFER_SIZE];
                ssize_t valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread <= 0) {
                    // Ensure client_addr and client_len are properly used here
                    getpeername(sd, (struct sockaddr *)&client_addr, &client_len);
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Message from client %d: %s\n", i, buffer);

                    // Check if it's a GET or POST request and handle accordingly
                    if (strstr(buffer, "GET") != NULL) {
                        // Handle GET request
                        // Modify this part to handle GET requests
                    } else if (strstr(buffer, "POST") != NULL) {
                        // Handle POST request
                        // Modify this part to handle POST requests
                    }

                    // Additional logic to handle the request can be added here
                }
            }
        }
    }
}
//TESTING
//To start do
// ./server on first terminal
// open another tab in terminal or open another terminal
// do either of commands to test

//GET
//first open chrome then write chrome://flags/ in url
//In the search bar, enter "HTTP/1.0" to find the option.
//In the "Enable HTTP/1.0" dropdown, select "Enabled."
//Click the "Relaunch" button at the bottom to apply the changes and restart Chrome.

//if firefox about:config
//You may see a warning message; click the "Accept the Risk and Continue" button.
//In the search bar at the top, enter "httpversion" to filter the results.
//You should see an option called "network.http.version." Double-click it to change the value.
//In the popup, enter "1.0" and click the "OK" button.

//GET
//curl -0 http://localhost:8080

//POST
//curl -X POST -d "key1=value1&key2=value2" http://127.0.0.1:8080/path
