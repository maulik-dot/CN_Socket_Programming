#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>


void *client_worker(void* args){

    // Extracting the server IP address from the arguments. 
    char* server_ip = *(char **)args;

    // Storing the thread ID for future reference.
    pthread_t thread_id = pthread_self();

    // Creating a socket for the client.
    int socket_1;
    if((socket_1 = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Thread %lu: Failed to generate socket. Terminating the thread.", thread_id);
        return NULL;
    }

    // Initialising the values for the bind and binding the socket.
    struct sockaddr_in server_dets;
    server_dets.sin_family = AF_INET;
    server_dets.sin_port = htons(8080);

    // Converting the IP address from human readible to machine understandable form.
    if (inet_pton(AF_INET, server_ip, &server_dets.sin_addr) <= 0) {
        printf("Thread %lu: Invalid address/Address not supported for conversion.\n", thread_id);
        return NULL;
    }

    // Connecting to the server at the given address.
    if (connect(socket_1, (struct sockaddr *)&server_dets, sizeof(server_dets)) < 0) {
        printf("Thread %lu: Connection Failed to the Server.\n", thread_id);
        return NULL;
    }

    // If connection successful sending the request to the server.
    const char *first_message = "Hello from Client. Please send the processes. ";
    send(socket_1, first_message, strlen(first_message), 0);
    printf("Thread %lu: First message sent to the server.\n", thread_id);

    char recv_buffer[3072] = {0};
    read(socket_1, recv_buffer, 3072);
    printf("Thread %lu: Message received: %s\n", thread_id, recv_buffer);

    close(socket_1);
    return NULL;
}



int main(int argc, char *argv[]) {

    // Checking of the number of arguments is equal to 2 or not. 

    if (argc != 3) {
        printf("Enter 3 parameters while executing. <executible name> <Server IP address> <Number of Children>\n");
        exit(EXIT_FAILURE);
    }

    // Storing the server IP and the number of children to spawn from the client.
    char* servIP = argv[1];
    int numProcesses = atoi(argv[2]);

    pthread_t thread_store[numProcesses];
    int return_code_check;

    for(int i = 0; i < numProcesses; i++){
        return_code_check = pthread_create(&thread_store[i], NULL, client_worker, &servIP);
        if(return_code_check){
            printf("Unable to make threads %d, return code: %d", i, return_code_check);
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < numProcesses; i++){
        pthread_join(thread_store[i], NULL);
    }

    printf("All threads completed.");
    return 0;
}
