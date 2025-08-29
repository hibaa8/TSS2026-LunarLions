///////////////////////////////////////////////////////////////////////////////////
//                                  Headers
///////////////////////////////////////////////////////////////////////////////////

#include "network.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//variables for rate limiting 
#define RECENT_CLIENT_CAPACITY 100
#define RATE_LIMIT_THRESHOLD 0.00f
#define RATE_LIMIT_THRESHOLD_DUST 0.00f //seconds

struct client_info_t* recent_clients[RECENT_CLIENT_CAPACITY] = {0}; //global circular buffer
//pointers for buffer
int lru_index = 0;
int recent_client_size = 0;

// struct profile_context_t {
//     void* pInternal;
// };

///////////////////////////////////////////////////////////////////////////////////
//                                 Clock Functions
///////////////////////////////////////////////////////////////////////////////////

void clock_setup(struct profile_context_t* ptContext) {
    #ifdef _WIN32
        static LARGE_INTEGER perf_freq;
        QueryPerformanceFrequency(&perf_freq);
        ptContext->pInternal = &perf_freq;

    #elif defined(__APPLE__)
        ptContext->pInternal = NULL;

    #else // Linux
        static struct timespec ts;
        static double dPerFrequency = 0;
    
        if (dPerFrequency == 0) { // Ensure it's initialized only once
            if (clock_getres(CLOCK_MONOTONIC, &ts) != 0) {
                fprintf(stderr, "clock_getres() failed\n");
                exit(1);
            }
            double total_nsec = (double)ts.tv_nsec + (double)ts.tv_sec * 1e9;
            if (total_nsec == 0) {
                fprintf(stderr, "Invalid clock resolution (division by zero)\n");
                exit(1);
            }
            dPerFrequency = 1e9 / total_nsec;
        }
    
        ptContext->pInternal = &dPerFrequency;
    #endif
}

///////////////////////////////////////////////////////////////////////////////////
//                                 Functions
///////////////////////////////////////////////////////////////////////////////////

void get_ip_address(char* hostname_out) {

    // initialize default hostname
    memset(hostname_out, 0, 16);
    strcpy(hostname_out, "127.0.0.1");

#if defined(_WIN32)
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return;
    }
    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            return;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            char* ipAddr = pAdapter->IpAddressList.IpAddress.String;
            if (strcmp(ipAddr, "127.0.0.1") != 0 && strcmp(ipAddr, "0.0.0.0") != 0) {
                memset(hostname_out, 0, 16);
                strcpy(hostname_out, ipAddr);
            }

            pAdapter = pAdapter->Next;
        }
    }
    else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

    }
    if (pAdapterInfo)
        free(pAdapterInfo);

#else
    // Get all addresses
    struct ifaddrs* addresses;
    if (getifaddrs(&addresses) == -1) {
        printf("getifaddrs call failed\n");
        return;
    }

    // Loop through addresses and find a non local host IP4 address
    struct ifaddrs* address = addresses;
    while (address) {
        // skip if the address is NULL
        if (address->ifa_addr == NULL) {
            address = address->ifa_next;
            continue;
        }

        // Check that it is IP4
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET) {

            char ap[100];
            const int family_size = sizeof(struct sockaddr_in);
            getnameinfo(address->ifa_addr, family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            // Check that it isn't local host
            if (strcmp(ap, "127.0.0.1") != 0) {
                memset(hostname_out, 0, 16);
                strcpy(hostname_out, ap);
            }

        }
        address = address->ifa_next;
    }
    freeifaddrs(addresses);
#endif
}

const char* get_content_type(const char* path){
    const char* last_dot = strstr(path, ".");
    if(last_dot){
        if(strcmp(last_dot, ".css") == 0)  return "text/css"; 
        if(strcmp(last_dot, ".csv") == 0)  return "text/csv"; 
        if(strcmp(last_dot, ".gif") == 0)  return "image/gif"; 
        if(strcmp(last_dot, ".htm") == 0)  return "text/html"; 
        if(strcmp(last_dot, ".html") == 0) return "text/html"; 
        if(strcmp(last_dot, ".ico") == 0)  return "image/x-icon"; 
        if(strcmp(last_dot, ".jpeg") == 0) return "image/jpeg"; 
        if(strcmp(last_dot, ".jpg") == 0)  return "image/jpeg"; 
        if(strcmp(last_dot, ".js") == 0)   return "application/javascript"; 
        if(strcmp(last_dot, ".json") == 0) return "application/json"; 
        if(strcmp(last_dot, ".png") == 0)  return "image/png"; 
        if(strcmp(last_dot, ".pdf") == 0)  return "application/pdf"; 
        if(strcmp(last_dot, ".svg") == 0)  return "image/svg+xml"; 
        if(strcmp(last_dot, ".txt") == 0)  return "text/plain"; 
    }

    return "application/octet-stream";
}

SOCKET create_udp_socket(char* hostname, char* port){

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(hostname);
    server_addr.sin_port= htons(atoi(port));

    printf("Creating UDP Socket...\n");
    SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1){
        perror("Failed to create socket.");

        return -1;
    }

    printf("Binding UDP Socket...\n");
    int bind_result = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(bind_result == -1){
        perror("Failed to bind...");

        return -1;
    }

    printf("Listening to UDP Socket...\n");

    return server_socket;
}

SOCKET create_socket(char* hostname, char* port){
   
    printf("Configuring Local Address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    struct addrinfo *bind_address;
    if(getaddrinfo(hostname, port, &hints, &bind_address)){
        fprintf(stderr, "getaddrinfo() failed with error: %d", GETSOCKETERRNO());
        exit(1);
    }

    printf("Creating HTTP Socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_listen)){
        fprintf(stderr, "socket() failed with error: %d", GETSOCKETERRNO());
        exit(1);
    }

    char c = 1;
    setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &c, sizeof(int));
    
    printf("Binding HTTP Socket...\n");
    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)){
        fprintf(stderr, "bind() failed with error: %d", GETSOCKETERRNO());
        CLOSESOCKET(socket_listen);
        exit(1);
    }

    printf("Listening to HTTP Socket...\n");
    if(listen(socket_listen, 10) > 0){
        fprintf(stderr, "listen() failed with error: %d", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);

    return socket_listen;

}

struct client_info_t* get_client(struct client_info_t** clients, SOCKET socket){
    
    // look for the client in the linked list
    struct client_info_t* client = *clients;
    while(client){

        if(client->socket == socket){
            return client;
        }
        client = client->next;
    }

    // client not found, make one
    struct client_info_t* new_client = (struct client_info_t*) malloc(sizeof(struct client_info_t));
    if(new_client == NULL){
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    // Set struct to zero, this is mainly for the char request buffer (since 0 is the NULL Terminator)
    memset(new_client, 0, sizeof(struct client_info_t));

    // Initialize local variables
    new_client->address_length = sizeof(new_client->address);
    new_client->received = 0;
    new_client->message_size = -1;

    // Push to clients list
    new_client->next = *clients;
    *clients = new_client;

    return new_client;


}

void drop_udp_client(struct client_info_t** clients, struct client_info_t* client){

    struct client_info_t** p = clients;

    while(*p){
        if(*p == client){
            *p = client->next;
            free(client);
            return;
        }
        p = & ((*p)->next);
    }

    fprintf(stderr, "drop_client: client not found\n");
    exit(1);

}

void drop_client(struct client_info_t** clients, struct client_info_t* client){
    
    CLOSESOCKET(client->socket);

    struct client_info_t** p = clients;

    while(*p){
        if(*p == client){
            *p = client->next;
            free(client);
            return;
        }
        p = & ((*p)->next);
    }

    fprintf(stderr, "drop_client: client not found\n");
    exit(1);

}

const char* get_client_address(struct client_info_t* client){
    static char address_buffer[100];
    getnameinfo((struct sockaddr*) &client->address, client->address_length, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    return address_buffer;
}

const char* get_client_udp_address(struct client_info_t* client){
    static char address_buffer[100];
    getnameinfo((struct sockaddr*) &client->udp_addr, client->address_length, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    return address_buffer;
}

fd_set wait_on_clients(struct client_info_t* clients, SOCKET server, SOCKET udp_socket){

    struct timeval select_wait;
    select_wait.tv_sec = 0;
    select_wait.tv_usec = 100000; 

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    FD_SET(udp_socket, &reads);
    SOCKET max_socket = server > udp_socket ? server : udp_socket;

    // Push all the clients to reads
    struct client_info_t* client = clients;
    while(client){
        FD_SET(client->socket, &reads);
        if(client->socket > max_socket){
            max_socket = client->socket;
        }
        client = client->next;
    }

    // Wait on sockets
    //printf("This select. \n");
    if(select(max_socket+1, &reads, 0, 0, &select_wait) < 0){
        fprintf(stderr, "select() failed with error: %d", GETSOCKETERRNO());
        exit(1);
    }

    return reads;

}

void send_400(struct client_info_t* client){  
    
    const char* c400 = "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 10\r\n\r\nBadRequest";

    send(client->socket, c400, strlen(c400), 0);

}

void send_404(struct client_info_t* client){  
    
    const char* c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nNot Found";

    send(client->socket, c404, strlen(c404), 0);
    
}

void send_201(struct client_info_t* client){  
    
    const char* c201 = "HTTP/1.1 201 Created\r\n"
        "Connection: close\r\n"
        "Content-Length: 7\r\n\r\nCreated";

    send(client->socket, c201, strlen(c201), 0);
    
}

void send_304(struct client_info_t* client){  
    
    const char* c304 = "HTTP/1.1 304 Not Modified\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 12\r\n\r\nNot Modified";

    int bytes_sent = send(client->socket, c304, strlen(c304), 0);
}

void reset_client_request_buffer(struct client_info_t* client){
    // This allows a client to send a new request over the same socket
    client->received = 0;
    memset(client->request, 0, MAX_REQUEST_SIZE);
}

void serve_resource(struct client_info_t* client, const char* path){

    if(strcmp(path, "/") == 0){
        path = "/index.html";
    }

    if(strlen(path) > 100){
        send_400(client);
        return;
    }

    if(strstr(path, "..")){
        send_404(client);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);

    // Swap '/' to '\' on Windows
    #if defined(_WIN32)
    char *p = full_path;
    while(*p) {
        if(*p == '/'){
            *p = '\\';
        }
        p++;
    }
    #endif

    // Get the requested file
    FILE *fp = fopen(full_path, "rb");

    // if not found
    if(!fp){
        send_404(client);
        return;
    }

    // if found 
    fseek(fp, 0L, SEEK_END);
    size_t content_length = ftell(fp);
    rewind(fp);

    const char* content_type = get_content_type(full_path);

    #define BSIZE 1024

    char content_buffer[BSIZE];

    // Send Header
    sprintf(content_buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, content_buffer, strlen(content_buffer), 0);

    sprintf(content_buffer, "Connection: close\r\n");
    send(client->socket, content_buffer, strlen(content_buffer), 0);

    sprintf(content_buffer, "Content-Length: %lu\r\n", content_length);
    send(client->socket, content_buffer, strlen(content_buffer), 0);

    sprintf(content_buffer, "Content-Type: %s\r\n", content_type);
    send(client->socket, content_buffer, strlen(content_buffer), 0);

    sprintf(content_buffer, "\r\n");
    send(client->socket, content_buffer, strlen(content_buffer), 0);

    // Send Content
    int bytes_read = fread(content_buffer, 1, BSIZE, fp);
    while(bytes_read){
        send(client->socket, content_buffer, bytes_read, 0);
        bytes_read = fread(content_buffer, 1, BSIZE, fp);
    }

    // Clean up
    fclose(fp);

}


/*Makes a copy of a client info struct*/
struct client_info_t* client_constructor(struct client_info_t* client) {
    if(!client) {
        return NULL;
    }
    struct client_info_t* new_client = malloc(sizeof(struct client_info_t));

    //copy variables
    new_client->last_request_time = client->last_request_time;
    new_client->address_length = client->address_length;
    new_client->address = client->address;
    new_client->udp_addr = client->udp_addr;
    new_client->socket = client->socket;
    new_client->received = client->received;
    new_client->message_size = client->message_size;

    //copy arrays
    memcpy(new_client->request, client->request, MAX_REQUEST_SIZE + 1);
    memcpy(new_client->udp_request, client->udp_request, MAX_UDP_REQUEST_SIZE);

    // //copy timestruct
    // if(client->ts) { //check that timespec exists before we allocate variables
    //     new_client->ts = malloc(sizeof(struct timespec));
    //     new_client->ts->tv_nsec = client->ts->tv_nsec;
    //     new_client->ts->tv_sec = client->ts->tv_sec;
    // } else {
    //     new_client->ts = NULL;
    // }

    //next isnt used in the context of rate limiting but setting it just in case
    new_client->next = client->next;

    return new_client;
}




/*Returns whether or not 2 clients are the same,
right now the code just checks whether or not 
sockets use the same IP, this can be changed tho*/
int compare_clients(struct client_info_t* client1, struct client_info_t* client2 ) {
    if(!client1 || !client2) {
        return 0;
    }

    if(client1->udp_addr.sin_family != client2->udp_addr.sin_family) {
        //if one is ipv4 and the other is ipv6 they are not the same
        return 0;
    }

    if(client1->udp_addr.sin_family == AF_INET) { //checks if both clients are IPV4

        //special struct just for ipv4
        struct sockaddr_in* addr1 = (struct sockaddr_in*)&client1->udp_addr;
        struct sockaddr_in* addr2 = (struct sockaddr_in*)&client2->udp_addr;

        //compare ipv4 addresses

        if(addr1->sin_addr.s_addr == addr2->sin_addr.s_addr) {
            return 1;  
        }
        
        //check if theyre same on 127.0.0.1
        if(addr1->sin_addr.s_addr == htonl(INADDR_LOOPBACK) ||
            addr2->sin_addr.s_addr == htonl(INADDR_LOOPBACK)) {
            return 1;  
        }

    } else if(client1->udp_addr.sin_family == AF_INET6){ //both must be IPV6

        //special struct just for ipv6
        struct sockaddr_in6* addr1 = (struct sockaddr_in6*)&client1->udp_addr;
        struct sockaddr_in6* addr2 = (struct sockaddr_in6*)&client2->udp_addr;
        
        //now we compare ipv6
        if(!memcmp(&addr1->sin6_addr, &addr2->sin6_addr, sizeof(struct in6_addr))) {
            return 1; 
        }

        //check 127.0.0.1
        struct in6_addr loopback = IN6ADDR_LOOPBACK_INIT;
        if(!memcmp(&addr1->sin6_addr, &loopback, sizeof(loopback)) ||
            !memcmp(&addr2->sin6_addr, &loopback, sizeof(loopback))) {
            return 1; 
        }
    }
    return 0;
}

/* Returns index in recent_clients array if the client is already there
/* or -1 otherwise
*/
int get_client_index(struct client_info_t* client) {
    //if its null it doesnt exist
    if (!client) {
        return -1;
    }

    //goes through every client in our list
    for (int i = 0; i < recent_client_size; i++) {
        if(compare_clients(recent_clients[i], client)) {
            //if they're the same we return the index at which its found
            return i;
        }
    }
    //otherwise we return -1 as an indicator that it wasnt found ):
    return -1;
}


/*This function adds a client to the recent clients buffer and returns whether or not it
was successfull*/
int add_client(struct client_info_t* client) {
    if (!client) return 0;  // null check first

    double time_now = get_wall_clock(&profile_context);
    struct client_info_t* new_client = client_constructor(client);
    new_client->last_request_time = time_now;

    if (recent_client_size == RECENT_CLIENT_CAPACITY) {
        recent_clients[lru_index] = new_client;
        lru_index = (lru_index + 1) % RECENT_CLIENT_CAPACITY;
    } else {
        recent_clients[recent_client_size] = new_client;
        recent_client_size++;
    }

    return 1;
}

/*Updates client time to new time (cause they just sent another message) and returns new time*/
double update_client_time(struct client_info_t* client) {
    if (!client) return -1;

    double time_now = get_wall_clock(&profile_context);
    client->last_request_time = time_now;

    int client_index = get_client_index(client);
    if (client_index != -1) {
        recent_clients[client_index] = client_constructor(client);
        recent_clients[client_index]->last_request_time = time_now;
    }

    return time_now;
}

//returns whether or not the client has sent a message within last threshold
int rate_limit_required(struct client_info_t* client, int dust_rate){

    float rate = (dust_rate == 0) ? RATE_LIMIT_THRESHOLD_DUST : RATE_LIMIT_THRESHOLD; //seconds

    // printf("rate limit: %f\n", rate);
    double time_now = get_wall_clock(&profile_context);
    int idx = get_client_index(client);
    if (idx == -1) {
        printf("client not found in recent_clients\n");
        return 0;
    }

    double last = recent_clients[idx]->last_request_time;
    double diff = time_now - last;

    return diff <= rate;
}

struct client_info_t* get_recent_client(int index) {
    return recent_clients[index];
}
