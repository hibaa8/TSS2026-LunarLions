///////////////////////////////////////////////////////////////////////////////////
//                                   Headers
///////////////////////////////////////////////////////////////////////////////////

// Standard C Stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <time.h>
#include "deps/cJSON.h"

// Networking Headers
#include "network.h"
struct profile_context_t profile_context;

// Application
#include "server_data.h"

// Server Variables
#define MAX_LINE_LENGHT 1024

#define TSS_TO_UNREAL_BRAKES_COMMAND 2000
#define TSS_TO_UNREAL_LIGHTS_COMMAND 2001
#define TSS_TO_UNREAL_STEERING_COMMAND 2002
#define TSS_TO_UNREAL_THROTTLE_COMMAND 2003
#define TSS_TO_UNREAL_SWITCH_DEST_COMMAND 2004

// Uncomment this for extra print statements
//#define VERBOSE_MODE 
//#define TESTING_MODE


///////////////////////////////////////////////////////////////////////////////////
//                      Helper Functions Declarations
///////////////////////////////////////////////////////////////////////////////////

bool continue_server();
//bool big_endian();
void get_contents();
//void reverse_bytes();
void tss_to_unreal();

///////////////////////////////////////////////////////////////////////////////////
//                               Main Functions
///////////////////////////////////////////////////////////////////////////////////

enum { NS_PER_SECOND = 1000000000 };

int main(int argc, char* argv[])
{

    int set_dust_rate;
    printf("Hello World\n\n");

    clock_setup(&profile_context);

    // Windows Specific Init
    #if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d)){

        printf("Failed to initialize WSA\n");
        return -1;
    }
    #endif
    FILE *fptr;

    // ----------------- Begin Main Program Space -------------------------
    // struct timespec time_begin, time_end, time_delta;

    double time_begin = get_wall_clock(&profile_context);

    //clock_gettime(CLOCK_REALTIME, &time_begin);

    bool udp_only = false;
    bool local = false;
    unsigned int old_time = 0;

    // Check for running in local host
    char hostname[16];
    char port[6] = "14141";
    
    for (int i = 0; i < argc; i++){
     if(strcmp(argv[i], "--udp") == 0){
            udp_only = true;
        }
        else if(strcmp(argv[i], "--local") == 0){
            local = true;
        }
    }

    if(local){
        strcpy(hostname, "127.0.0.1");
    }
    else{
        get_ip_address(hostname);
    }

    printf("Launching Server at IP: %s:%s\n", hostname, port);

    SOCKET server;
    SOCKET udp_socket;

    struct sockaddr_in unreal_addr;
    socklen_t unreal_addr_len;
    bool unreal = false;

    // Create server sockets 
    if(udp_only){
        udp_socket = create_udp_socket(hostname, port);
    }
    else{
        server = create_socket(hostname, port);
        udp_socket = create_udp_socket(hostname, port);
    }

    #ifdef TESTING_MODE
        printf("\nBig-endian system: %s\n", big_endian() ? "yes" : "no");
    #endif

    // "Data Base" Data
    struct backend_data_t* backend = init_backend(backend);

    // Client connection Data
    struct client_info_t* clients = NULL;
    //////////////////////////////////////////////////////////////////// Server /////////////////////////////////////////////////////////////////////////////////
    while(true){
        fd_set reads;
        reads = wait_on_clients(clients, server, udp_socket);

        // Server Listen Socket got a new message
        if(FD_ISSET(server, &reads)){

            struct client_info_t* client = get_client(&clients, -1);

            // create client socket
            client->socket = accept(server, (struct sockaddr*) &client->address, &client->address_length);
            if(!ISVALIDSOCKET(client->socket)){
                fprintf(stderr, "accept() failed with error: %d", GETSOCKETERRNO());
            }
            
            

            #ifdef VERBOSE_MODE
            if(strcmp(get_client_address(client), hostname)){
                printf("New Connection from %s\n", get_client_address(client));

            }
            #endif

        }



        // Handle UDP
        if(FD_ISSET(udp_socket, &reads)){
            struct client_info_t* udp_clients = NULL;
            struct client_info_t* client = get_client(&udp_clients, -1);

            int received_bytes = recvfrom(udp_socket, client->udp_request, MAX_UDP_REQUEST_SIZE, 0, (struct sockaddr*)&client->udp_addr, &client->address_length);
            if(!big_endian()){
                reverse_bytes(client->udp_request);
                reverse_bytes(client->udp_request + 4);
                reverse_bytes(client->udp_request + 8);   
            }

            unsigned int time = 0;
            unsigned int command = 0;
            char data[4] = {0};
            // char* dust_ip = "192.168.51.201";
            
      
            // printf("UDP IP: %s\n", get_client_udp_address(client));

            get_contents(client->udp_request, &time, &command, data);

            //if(!((command >= 1100 && command <= 1130) || command == 3000 || command == 0)) {
            // if((strcmp(get_client_udp_address(client), dust_ip) != 0)){
            //     printf("Command: %d\n", command);
                
            //     int client_index = get_client_index(client); //check if our client is new or not

            //     if (!client) {
            //         fprintf(stderr, "[ERROR] NULL client passed to rate_limit_required\n");
            //     }
                
            //     if(client_index == -1) { //case that client isnt stored yet
                    
            //         //add it to our list and then update its time
            //         add_client(client);
            //         update_client_time(client);

            //     } else { //case that it is stored

            //         set_dust_rate = (((command >= 1100 && command <= 1130) || command == 3000 || command == 0)) ? 0 : 1; //if we are getting a dust command we set the rate to 0, otherwise we set it to 1
                    
            //         if(rate_limit_required(client, set_dust_rate)) { //if we need to rate limit we dont update the time
                        
            //             printf("Rate Limit hit\n");
            //             continue; //this drops the udp message

            //         } else {

            //             //otherwise the client is able to send so we just update its' time
            //             update_client_time(client);
            //         }

            //     }
            // }
  

            
            

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

            //check if it's a GET request
            if (command < 1000){
#ifdef VERBOSE_MODE
                printf("Received a GET request from %s:%d \n", inet_ntoa(client->udp_addr.sin_addr), ntohs(client->udp_addr.sin_port));
#endif
                unsigned char* response_buffer;
                int buffer_size = 0;

                //Send lidar
                if(command == 172 && backend->running_pr_sim >= 0){
                    response_buffer = malloc(sizeof(backend->p_rover[backend->running_pr_sim].lidar) + 8);
                    buffer_size = sizeof(backend->p_rover[backend->running_pr_sim].lidar) + 8;

                    udp_get_pr_lidar(response_buffer + 8, backend);

                    memcpy(response_buffer, &backend->server_up_time, 4);
                    memcpy(response_buffer + 4, &command, 4);

                    if(!big_endian()){
                        reverse_bytes(response_buffer);
                        reverse_bytes(response_buffer + 4);
                    }

                }
                //Handle normal GET request
                else{

                    handle_udp_get_request(command, data, backend);
                    response_buffer = malloc(12);
                    buffer_size = 12;

                    memcpy(response_buffer, &backend->server_up_time, 4);
                    memcpy(response_buffer + 4, &command, 4);
                    memcpy(response_buffer + 8, data, 4);

                    if(!big_endian()){
                        reverse_bytes(response_buffer);
                        reverse_bytes(response_buffer + 4);
                        reverse_bytes(response_buffer + 8);
                    }
                }

                //Send response
                int bytes_sent = sendto(udp_socket, response_buffer, buffer_size, 0, (struct sockaddr*)&client->udp_addr, client->address_length);

#ifdef VERBOSE_MODE
                printf("Sent response to %s:%d\n", inet_ntoa(client->udp_addr.sin_addr), ntohs(client->udp_addr.sin_port));
                printf("Bytes sent: %d\n", bytes_sent);
#endif
                drop_udp_client(&udp_clients, client);
                free(response_buffer);

            }
            //check if it's a POST request
            else if (command < 2000){
                
#ifdef VERBOSE_MODE
                printf("Received a POST request from %s:%d \n", inet_ntoa(client->udp_addr.sin_addr), ntohs(client->udp_addr.sin_port));
#endif          
                handle_udp_post_request(command, data, client->udp_request, backend, received_bytes);

                drop_udp_client(&udp_clients, client);
            }
            //Save address of Unreal
            else if (command == 3000){

                unreal_addr = client->udp_addr;
                unreal_addr_len = client->address_length;
                unreal = true;

                //#ifdef TESTING_MODE
                    printf("Unreal address set to %s:%d\n", inet_ntoa(client->udp_addr.sin_addr), ntohs(client->udp_addr.sin_port));
                //#endif

                drop_udp_client(&udp_clients, client);
            }

            else {
                drop_udp_client(&udp_clients, client);
            }
        }
        
        // Send telemetry values to Unreal if there's an address saved
        if(unreal){
            double time_end = get_wall_clock(&profile_context);
            if ((time_end - time_begin) > 0.2) {
                tss_to_unreal(udp_socket, unreal_addr, unreal_addr_len, backend);
                time_begin = time_end;
            }
        }

        // Server-Client Socket got a new message
        struct client_info_t* client = clients;

        if(!udp_only){

            while(client){
                
                struct client_info_t* next_client = client->next;

                // Check if this client has pending request
                if(FD_ISSET(client->socket, &reads)){

                    // Request too big
                    if(MAX_REQUEST_SIZE <= client->received){
                        send_400(client);
                        drop_client(&clients, client);
                        client = next_client;
                        continue;
                    }

                    // read new bytes in
                    int bytes_received = recv(client->socket, client->request + client->received, MAX_REQUEST_SIZE - client->received, 0);

                    if(bytes_received < 1){
                        
                        #ifdef VERBOSE_MODE
                        fprintf(stderr, "Unexpected Disconnect from %s\n", get_client_address(client));
                        #endif
                        drop_client(&clients, client);
                    } else {

                        if(strncmp(client->request, "GET/", 4) == 0){
#ifdef VERBOSE_MODE
                            printf("UDP request\n");
#endif
                        }

                        client->received += bytes_received;
                        client->request[client->received] = 0;

                        // Find marker for the end of the header
                        char* q = strstr(client->request, "\r\n\r\n");
                        if(q){

                            // Received a GET Request
                            if(strncmp(client->request, "GET /", 5) == 0){
                                char* path = client->request + 4;
                                char* end_path = strstr(path, " ");

                                if(!end_path){
                                    send_400(client);
                                    drop_client(&clients, client);
                                } else {
                                    *end_path = 0;
                                    serve_resource(client, path);
                                    #ifdef VERBOSE_MODE
                                    printf("serve_resource %s %s\n", get_client_address(client), path);
                                    #endif
                                    drop_client(&clients, client);
                                }
                            } // Received a POST Request 
                            else if(strncmp(client->request, "POST /", 6) == 0) {

                                // Get the size of the post request
                                if(client->message_size == -1){
                                    char* request_content_size_ptr = strstr(client->request, "Content-Length: ");
                                    if(request_content_size_ptr){
                                        request_content_size_ptr += strlen("Content-Length: ");
                                        client->message_size = atoi(request_content_size_ptr) + (q - client->request) + 4; // The size of the content plus size of the header
                                    } else {
                                        // There is no content size
                                        send_400(client);
                                        drop_client(&clients, client);
                                    }
                                } 
                                
                                if(client->received == client->message_size){

                                    // all bytes loaded
                                    char* request_content = strstr(client->request, "\r\n\r\n");
                                    request_content += 4; // Jump to the beginning of the context
                                
                                    #ifdef VERBOSE_MODE
                                    printf("Received Post Content: \n%s\n", request_content);
                                    #endif

                                    if(!request_content){
                                        send_400(client);
                                        drop_client(&clients, client);
                                    } else {
                                        if(update_resource(request_content, backend)){
                                            send_304(client);
                                        } else {
                                            send_400(client);
                                        }
                                        drop_client(&clients, client);
                                    }
                                }

                            } // Received some other request
                            else {
                                send_400(client);
                                drop_client(&clients, client);
                            }
                        }
                    }
                }

                client = next_client;
            }
        }

        if(!continue_server()) {
            break;
        }

        // Simulate the variables
        simulate_backend(backend);

    }


    // Clean up
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
    struct client_info_t* client = clients;
    while(client){
        drop_client(&clients, client);
        leftover_clients++;
        client = client->next;
    }
    printf("Cleaned up %d client sockets\n", leftover_clients);

    // ------------------ End Main Program Space -------------------------

    // Windows Specific Cleanup
    #if defined(_WIN32)
    WSACleanup();
    #endif

    printf("\nGoodbye World\n");
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////
//                             Helper Functions
///////////////////////////////////////////////////////////////////////////////////

bool continue_server(){

    // if the user presses the ENTER key, the program will end gracefully
    struct timeval select_wait;
    select_wait.tv_sec = 0;
    select_wait.tv_usec = 0; 
    fd_set console_fd;
    FD_ZERO(&console_fd);
    FD_SET(0, &console_fd);
    int read_files = select(1, &console_fd, 0,0, &select_wait);
    if(read_files > 0) {
        return false;
    } else {
        return true;
    }
}

void get_contents(char* buffer, unsigned int* time, unsigned int* command, unsigned char* data){
    memcpy(time, buffer, 4);
    memcpy(command, buffer + 4, 4);
    memcpy(data, buffer + 8, 4);
}

void tss_to_unreal(int socket, struct sockaddr_in address, socklen_t len, struct backend_data_t* backend){
    if (backend->running_pr_sim < 0) {
        return;
    }
    int brakes = backend->p_rover[backend->running_pr_sim].brakes;
    int lights_on = backend->p_rover[backend->running_pr_sim].lights_on;
    float steering = backend->p_rover[backend->running_pr_sim].steering;
    float throttle = backend->p_rover[backend->running_pr_sim].throttle;
    int switch_dest = backend->p_rover[backend->running_pr_sim].switch_dest;

    unsigned int time = backend->server_up_time;
    unsigned int command = TSS_TO_UNREAL_BRAKES_COMMAND;
    unsigned int command2 = TSS_TO_UNREAL_LIGHTS_COMMAND;
    unsigned int command3 = TSS_TO_UNREAL_STEERING_COMMAND;
    unsigned int command4 = TSS_TO_UNREAL_THROTTLE_COMMAND;
    unsigned int command5 = TSS_TO_UNREAL_SWITCH_DEST_COMMAND;

    unsigned char buffer[12];
    unsigned char buffer2[12];
    unsigned char buffer3[12];
    unsigned char buffer4[12];
    unsigned char buffer5[12];
    
    if(!big_endian()){
        reverse_bytes((unsigned char*)&time);
        reverse_bytes((unsigned char*)&command);
        reverse_bytes((unsigned char*)&command2);
        reverse_bytes((unsigned char*)&command3);
        reverse_bytes((unsigned char*)&command4);
        reverse_bytes((unsigned char*)&command5);
        reverse_bytes((unsigned char*)&steering);
        reverse_bytes((unsigned char*)&throttle);
    }
    
    memcpy(buffer, &time, 4);
    memcpy(buffer + 4, &command, 4);
    memcpy(buffer + 8, &brakes, 4);

    sendto(socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&address, len);

    memcpy(buffer2, &time, 4);
    memcpy(buffer2 + 4, &command2, 4);
    memcpy(buffer2 + 8, &lights_on, 4);

    sendto(socket, buffer2, sizeof(buffer2), 0, (struct sockaddr*)&address, len);

    memcpy(buffer3, &time, 4);
    memcpy(buffer3 + 4, &command3, 4);
    memcpy(buffer3 + 8, &steering, 4);

    sendto(socket, buffer3, sizeof(buffer3), 0, (struct sockaddr*)&address, len);

    memcpy(buffer4, &time, 4);
    memcpy(buffer4 + 4, &command4, 4);
    memcpy(buffer4 + 8, &throttle, 4);

    sendto(socket, buffer4, sizeof(buffer4), 0, (struct sockaddr*)&address, len);

    memcpy(buffer5, &time, 4);
    memcpy(buffer5 + 4, &command5, 4);
    memcpy(buffer5 + 8, &switch_dest, 4);

    sendto(socket, buffer5, sizeof(buffer5), 0, (struct sockaddr*)&address, len);
}