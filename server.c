// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MYPORT 3500
#define BUFFER_SIZE 100

// Structure to hold client information
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info;

// Function to handle client communication
void* handle_client(void* arg) {
    client_info* cinfo = (client_info*)arg;
    char buf[BUFFER_SIZE];
    int bytes_read;

    // Read message from client
    while ((bytes_read = read(cinfo->client_socket, buf, BUFFER_SIZE - 1)) > 0) {
        buf[bytes_read] = '\0';  // Null terminate the string
        printf("Message from client (%s:%d): %s\n",
               inet_ntoa(cinfo->client_addr.sin_addr),
               ntohs(cinfo->client_addr.sin_port),
               buf);
    }

    // Close the client socket
    close(cinfo->client_socket);
    free(cinfo); // Free the allocated memory for client_info
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in my_addr, con_addr;
    socklen_t struct_size = sizeof(con_addr);
    pthread_t tid;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind socket
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    listen(sockfd, 5);
    printf("Server listening on port %d\n", MYPORT);

    // Main loop to accept connections
    while (1) {
        // Accept a new client connection
        int client_sock = accept(sockfd, (struct sockaddr*)&con_addr, &struct_size);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Print client's IP address and port
        printf("Client connected: %s:%d\n", inet_ntoa(con_addr.sin_addr), ntohs(con_addr.sin_port));

        // Allocate memory for client_info structure
        client_info* cinfo = malloc(sizeof(client_info));
        if (cinfo == NULL) {
            perror("Failed to allocate memory for client info");
            close(client_sock);
            continue;
        }

        // Populate client_info structure
        cinfo->client_socket = client_sock;
        cinfo->client_addr = con_addr;

        // Create a new thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, cinfo) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            free(cinfo);
        }
        printf("Tid-");
        printf("%ld \n",tid);

        // Detach the thread to allow it to clean up after itself
        pthread_detach(tid);
    }

    // Close the server socket (unreachable in this example)
    close(sockfd);
    return 0;
}
