#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

    // Check if the correct number of arguments is passed
    if (argc != 2) {
        printf("Usage: %s <Server IP Address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Store the server IP address
    char *servIP = argv[1];

    // Create a socket
    int socket_1;
    if ((socket_1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the server address structure
    struct sockaddr_in server_dets;
    server_dets.sin_family = AF_INET;
    server_dets.sin_port = htons(8080);

    // Convert IP address from text to binary format
    if (inet_pton(AF_INET, servIP, &server_dets.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(socket_1, (struct sockaddr *)&server_dets, sizeof(server_dets)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Send a message to the server
    const char *first_message = "Hello from Client. Please send the processes.";
    send(socket_1, first_message, strlen(first_message), 0);
    printf("Message sent to the server: %s\n", first_message);

    // Receive the response from the server
    char recv_buffer[3072] = {0};
    read(socket_1, recv_buffer, sizeof(recv_buffer));
    printf("Message received from the server: %s\n", recv_buffer);

    // Close the socket
    close(socket_1);
    return 0;
}

