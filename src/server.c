// server.c - Main application server handling TCP/UDP connections, manages telemetry data, web interface, and Unreal Engine communication

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/cjson/cJSON.h"

#include "network.h"
#include "data.h"

struct profile_context_t profile_context;


#define TSS_TO_UNREAL_BRAKES_COMMAND 2000
#define TSS_TO_UNREAL_LIGHTS_COMMAND 2001
#define TSS_TO_UNREAL_STEERING_COMMAND 2002
#define TSS_TO_UNREAL_THROTTLE_COMMAND 2003
#define TSS_TO_UNREAL_SWITCH_DEST_COMMAND 2004

static bool continue_server(void);
static void get_contents(char *buffer, unsigned int *time, unsigned int *command, unsigned char *data);
static void tss_to_unreal(SOCKET socket, struct sockaddr_in address, socklen_t len, struct backend_data_t *backend);

#define UNREAL_UPDATE_INTERVAL_SEC 0.2

int main(int argc, char *argv[]) {
    printf("Hello World\n\n");

    clock_setup(&profile_context);

// Windows Specific Init
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        printf("Failed to initialize WSA\n");
        return -1;
    }
#endif

    double time_begin = get_wall_clock(&profile_context);

    bool udp_only = false;
    bool local = false;

    // Parse command line arguments
    char hostname[16];
    char port[6] = "14141";

for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--udp") == 0) {
            udp_only = true;
        } else if (strcmp(argv[i], "--local") == 0) {
            local = true;
        }
    }

    if (local) {
        strcpy(hostname, "127.0.0.1");
    } else {
        get_ip_address(hostname);
    }

    printf("Launching Server at IP: %s:%s\n", hostname, port);

    SOCKET server;
    SOCKET udp_socket;

    struct sockaddr_in unreal_addr;
    socklen_t unreal_addr_len;
    bool unreal = false;

    // Create server sockets
    if (udp_only) {
        udp_socket = create_udp_socket(hostname, port);
    } else {
        server = create_tcp_socket(hostname, port);
        udp_socket = create_udp_socket(hostname, port);
    }


    // Initialize backend data system
    struct backend_data_t *backend = init_backend(NULL);
    if (!backend) {
        fprintf(stderr, "Failed to initialize backend\n");
        return -1;
    }

    // Initialize client connection list
    struct client_info_t *clients = NULL;
    // Main server loop
    while (true) {
        fd_set reads;
        reads = wait_on_clients(clients, server, udp_socket);

        // Handle new TCP client connections
        if (FD_ISSET(server, &reads)) {
            struct client_info_t *client = get_client(&clients, -1);
            if (!client) {
                fprintf(stderr, "Failed to allocate memory for new client connection\n");
                continue;
            }

            // Accept the new connection
            client->socket =
                accept(server, (struct sockaddr *)&client->address, &client->address_length);
            if (!ISVALIDSOCKET(client->socket)) {
                fprintf(stderr, "accept() failed with error: %d\n", GETSOCKETERRNO());
                drop_tcp_client(&clients, client);
                continue;
            }

#ifdef VERBOSE_MODE
            if (strcmp(get_client_address(client), hostname)) {
                printf("New Connection from %s\n", get_client_address(client));
            }
#endif
        }

        // Handle UDP datagram packets
        if (FD_ISSET(udp_socket, &reads)) {
            struct client_info_t *udp_clients = NULL;
            struct client_info_t *client = get_client(&udp_clients, -1);
            if (!client) {
                fprintf(stderr, "Failed to allocate memory for UDP client\n");
                continue;
            }

            int received_bytes =
                recvfrom(udp_socket, client->udp_request, MAX_UDP_REQUEST_SIZE, 0,
                         (struct sockaddr *)&client->udp_addr, &client->address_length);
            if (!big_endian()) {
                reverse_bytes(client->udp_request);
                reverse_bytes(client->udp_request + 4);
                reverse_bytes(client->udp_request + 8);
            }

            unsigned int time = 0;
            unsigned int command = 0;
            char data[4] = {0};

            get_contents(client->udp_request, &time, &command, data);


#ifdef TESTING_MODE
            printf("\nNew datagram received.\n");

            printf("time: %d, ", time);
            printf("command: %d, ", command);
            float value = 0;
            memcpy(&value, data, 4);
            printf("data float: %f, ", value);
            int valuei = 0;
            memcpy(&valuei, data, 4);
            printf("data int: %d\n\n", valuei);
#endif

            // Process UDP command based on command range
            if (command < 1000) {  // GET requests
#ifdef VERBOSE_MODE
                printf("Received a GET request from %s:%d \n", inet_ntoa(client->udp_addr.sin_addr),
                       ntohs(client->udp_addr.sin_port));
#endif
                unsigned char *response_buffer;
                int buffer_size = 0;

                // Handle special LIDAR data request
                // @TODO look into the multiple ways that are currently available to fetch lidar
                if (command == 172 && backend->running_pr_sim >= 0) {
                    buffer_size = sizeof(backend->p_rover[backend->running_pr_sim].lidar) + 8;
                    response_buffer = malloc(buffer_size);
                    if (!response_buffer) {
                        fprintf(stderr, "Failed to allocate memory for lidar response\n");
                        drop_udp_client(&udp_clients, client);
                        continue;
                    }

                    udp_get_pr_lidar(response_buffer + 8, backend);

                    memcpy(response_buffer, &backend->server_up_time, 4);
                    memcpy(response_buffer + 4, &command, 4);

                    if (!big_endian()) {
                        reverse_bytes(response_buffer);
                        reverse_bytes(response_buffer + 4);
                    }

                }

                // Handle normal GET request
                else {
                    handle_udp_get_request(command, data, backend);
                    buffer_size = 12;
                    response_buffer = malloc(buffer_size);
                    if (!response_buffer) {
                        fprintf(stderr, "Failed to allocate memory for GET response\n");
                        drop_udp_client(&udp_clients, client);
                        continue;
                    }

                    memcpy(response_buffer, &backend->server_up_time, 4);
                    memcpy(response_buffer + 4, &command, 4);
                    memcpy(response_buffer + 8, data, 4);

                    if (!big_endian()) {
                        reverse_bytes(response_buffer);
                        reverse_bytes(response_buffer + 4);
                        reverse_bytes(response_buffer + 8);
                    }
                }

                // Send response
                int bytes_sent =
                    sendto(udp_socket, response_buffer, buffer_size, 0,
                           (struct sockaddr *)&client->udp_addr, client->address_length);

#ifdef VERBOSE_MODE
                printf("Sent response to %s:%d\n", inet_ntoa(client->udp_addr.sin_addr),
                       ntohs(client->udp_addr.sin_port));
                printf("Bytes sent: %d\n", bytes_sent);
#endif
                drop_udp_client(&udp_clients, client);
                free(response_buffer);

            }
            else if (command < 2000) {  // POST requests
#ifdef VERBOSE_MODE
                printf("Received a POST request from %s:%d \n",
                       inet_ntoa(client->udp_addr.sin_addr), ntohs(client->udp_addr.sin_port));
#endif
                handle_udp_post_request(command, data, client->udp_request, backend,
                                        received_bytes);

                drop_udp_client(&udp_clients, client);
            }
            else if (command == 3000) {  // Unreal Engine registration
                unreal_addr = client->udp_addr;
                unreal_addr_len = client->address_length;
                unreal = true;

                printf("Unreal address set to %s:%d\n", inet_ntoa(client->udp_addr.sin_addr),
                       ntohs(client->udp_addr.sin_port));

                drop_udp_client(&udp_clients, client);
            }

            else {  // Unknown command
                drop_udp_client(&udp_clients, client);
            }
        }

        // Send periodic telemetry updates to Unreal Engine
        if (unreal) {
            double time_end = get_wall_clock(&profile_context);
            if ((time_end - time_begin) > UNREAL_UPDATE_INTERVAL_SEC) {
                tss_to_unreal(udp_socket, unreal_addr, unreal_addr_len, backend);
                time_begin = time_end;
            }
        }

        // Handle existing TCP client requests
        struct client_info_t *client = clients;

        if (!udp_only) {
            while (client) {
                struct client_info_t *next_client = client->next;

                // Process data from this client
                if (FD_ISSET(client->socket, &reads)) {
                    // Reject oversized requests
                    if (MAX_REQUEST_SIZE <= client->received) {
                        send_400(client);
                        drop_tcp_client(&clients, client);
                        client = next_client;
                        continue;
                    }

                    // Read incoming data from client
                    int bytes_received = recv(client->socket, client->request + client->received,
                                              MAX_REQUEST_SIZE - client->received, 0);

                    if (bytes_received < 1) {
#ifdef VERBOSE_MODE
                        fprintf(stderr, "Unexpected Disconnect from %s\n",
                                get_client_address(client));
#endif
                        drop_tcp_client(&clients, client);
                    } else {

                        client->received += bytes_received;
                        client->request[client->received] = 0;

                        // Check if we have a complete HTTP request
                        char *q = strstr(client->request, "\r\n\r\n");
                        if (q) {
                            // Handle HTTP GET request
                            if (strncmp(client->request, "GET /", 5) == 0) {
                                char *path = client->request + 4;
                                char *end_path = strstr(path, " ");

                                if (!end_path) {
                                    send_400(client);
                                    drop_tcp_client(&clients, client);
                                } else {
                                    *end_path = 0;
                                    serve_resource(client, path);
#ifdef VERBOSE_MODE
                                    printf("serve_resource %s %s\n", get_client_address(client),
                                           path);
#endif
                                    drop_tcp_client(&clients, client);
                                }
                            }
                            // Handle HTTP POST request
                            else if (strncmp(client->request, "POST /", 6) == 0) {
                                // Parse Content-Length header
                                if (client->message_size == -1) {
                                    char *request_content_size_ptr =
                                        strstr(client->request, "Content-Length: ");
                                    if (request_content_size_ptr) {
                                        request_content_size_ptr += strlen("Content-Length: ");
                                        client->message_size = atoi(request_content_size_ptr) + 
                                                              (q - client->request) + 4;
                                    } else {
                                        // Missing Content-Length header
                                        send_400(client);
                                        drop_tcp_client(&clients, client);
                                    }
                                }

                                if (client->received == client->message_size) {
                                    // Complete POST request received
                                    char *request_content = strstr(client->request, "\r\n\r\n");
                                    request_content += 4;  // Skip past header delimiter

#ifdef VERBOSE_MODE
                                    printf("Received Post Content: \n%s\n", request_content);
#endif

                                    if (!request_content) {
                                        send_400(client);
                                        drop_tcp_client(&clients, client);
                                    } else {
                                        if (update_resource(request_content, backend)) {
                                            send_304(client);
                                        } else {
                                            send_400(client);
                                        }
                                        drop_tcp_client(&clients, client);
                                    }
                                }

                            }
                            // Handle unsupported HTTP methods
                            else {
                                send_400(client);
                                drop_tcp_client(&clients, client);
                            }
                        }
                    }
                }

                client = next_client;
            }
        }

        if (!continue_server()) {
            break;
        }

        // Update simulation state
        simulate_backend(backend);
    }

    // Cleanup phase - shutdown server gracefully
    printf("Clean up Database...\n");
    cleanup_backend(backend);

    printf("Closing Sockets...\n");
    CLOSESOCKET(server);
#if defined(_WIN32)
#else
    close(udp_socket);
#endif

    printf("Cleaned up server listen sockets\n");
    int leftover_clients = 0;
    struct client_info_t *client = clients;
    while (client) {
        drop_tcp_client(&clients, client);
        leftover_clients++;
        client = client->next;
    }
    printf("Cleaned up %d client sockets\n", leftover_clients);

    // Platform-specific cleanup
#if defined(_WIN32)
    WSACleanup();
#endif

    printf("\nGoodbye World\n");
    return 0;
}

/**
 * Checks if the user wants to stop the server by pressing ENTER.
 * Non-blocking check using select() with zero timeout.
 * @return false if ENTER was pressed, true to continue running
 */
static bool continue_server(void) {

    struct timeval select_wait;
    select_wait.tv_sec = 0;
    select_wait.tv_usec = 0;
    fd_set console_fd;
    FD_ZERO(&console_fd);
    FD_SET(0, &console_fd);
    int read_files = select(1, &console_fd, 0, 0, &select_wait);
    if (read_files > 0) {
        return false;
    } else {
        return true;
    }
}

/**
 * Extracts UDP packet contents into separate fields.
 * UDP packet format: [time:4][command:4][data:4]
 * @param buffer Raw UDP packet data
 * @param time Pointer to store timestamp
 * @param command Pointer to store command code  
 * @param data Pointer to store 4-byte data payload
 */
static void get_contents(char *buffer, unsigned int *time, unsigned int *command, unsigned char *data) {
    memcpy(time, buffer, 4);
    memcpy(command, buffer + 4, 4);
    memcpy(data, buffer + 8, 4);
}

/**
 * Sends telemetry data to Unreal Engine via UDP packets.
 * Transmits rover state (brakes, lights, steering, throttle, switch) as separate packets.
 * @param socket UDP socket for transmission
 * @param address Unreal Engine's network address
 * @param len Length of address structure
 * @param backend Backend data containing rover state
 */
static void tss_to_unreal(SOCKET socket, struct sockaddr_in address, socklen_t len,
                         struct backend_data_t *backend) {
    if (backend->running_pr_sim < 0) {
        return;
    }
    
    // Extract current rover state
    int brakes = backend->p_rover[backend->running_pr_sim].brakes;
    int lights_on = backend->p_rover[backend->running_pr_sim].lights_on;
    float steering = backend->p_rover[backend->running_pr_sim].steering;
    float throttle = backend->p_rover[backend->running_pr_sim].throttle;
    int switch_dest = backend->p_rover[backend->running_pr_sim].switch_dest;

    unsigned int time = backend->server_up_time;
    unsigned char buffer[12];
    
    // Convert endianness if needed
    if (!big_endian()) {
        reverse_bytes((unsigned char *)&time);
        reverse_bytes((unsigned char *)&steering);
        reverse_bytes((unsigned char *)&throttle);
    }

    // Send brakes command
    unsigned int command = TSS_TO_UNREAL_BRAKES_COMMAND;
    if (!big_endian()) reverse_bytes((unsigned char *)&command);
    memcpy(buffer, &time, 4);
    memcpy(buffer + 4, &command, 4);
    memcpy(buffer + 8, &brakes, 4);
    sendto(socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, len);

    // Send lights command
    command = TSS_TO_UNREAL_LIGHTS_COMMAND;
    if (!big_endian()) reverse_bytes((unsigned char *)&command);
    memcpy(buffer, &time, 4);
    memcpy(buffer + 4, &command, 4);
    memcpy(buffer + 8, &lights_on, 4);
    sendto(socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, len);

    // Send steering command
    command = TSS_TO_UNREAL_STEERING_COMMAND;
    if (!big_endian()) reverse_bytes((unsigned char *)&command);
    memcpy(buffer, &time, 4);
    memcpy(buffer + 4, &command, 4);
    memcpy(buffer + 8, &steering, 4);
    sendto(socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, len);

    // Send throttle command
    command = TSS_TO_UNREAL_THROTTLE_COMMAND;
    if (!big_endian()) reverse_bytes((unsigned char *)&command);
    memcpy(buffer, &time, 4);
    memcpy(buffer + 4, &command, 4);
    memcpy(buffer + 8, &throttle, 4);
    sendto(socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, len);

    // Send switch destination command
    command = TSS_TO_UNREAL_SWITCH_DEST_COMMAND;
    if (!big_endian()) reverse_bytes((unsigned char *)&command);
    memcpy(buffer, &time, 4);
    memcpy(buffer + 4, &command, 4);
    memcpy(buffer + 8, &switch_dest, 4);
    sendto(socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, len);
}