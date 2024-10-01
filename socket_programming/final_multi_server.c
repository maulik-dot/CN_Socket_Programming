#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>

// Define the data structure to hold process information that will be passed by the method
struct processes_data {
    int pid;
    char name[512];
    long unsigned int total_cpu_time; 
};

// Defining a custom comparator as I want to sort the processes in reverse order.
int compare_processes_comparator(const void *a, const void *b) {
    
    struct processes_data *procA = (struct processes_data *)a;
    struct processes_data *procB = (struct processes_data *)b;
    return (procB->total_cpu_time - procA->total_cpu_time);  
}

// Method for finding the top 2 cpu consuming processes.
struct processes_data* find_top_processes(){

    // Opening the proc directory and storing the pointer to the directory system.
    DIR *proc_dir = opendir("/proc");


    // Verifying if the proc_dir is alreaedy opened or not.
    if(proc_dir == NULL){
        perror("Unable to open the /proc directory. Terminating execution.");
        exit(EXIT_FAILURE);
    }


    struct processes_data processes_store[2048]; // Data structure for storing the processes. We are assuming max of 2048 processes.
    int process_counter = 0; // Variable storing the number of processes.
    
    // Iterating through the directory that we opened.
    struct dirent *entry;

    //Iterating for all the processes.
    while((entry = readdir(proc_dir)) != NULL){
        
        // Checking if the sub-directory name is a number or not.
        if(isdigit(*entry->d_name)){
            
            // Storing the path to the stat file in the data structure for a process.
            char stat_file_path[512];
            snprintf(stat_file_path, sizeof(stat_file_path), "/proc/%s/stat", entry->d_name);

            FILE *stats_file = fopen(stat_file_path, "r");

            // Checking if the file is empty or not.
            if(stats_file == NULL){
                // Skip the next processing if the file is empty.
                continue;
            }

            // Declaring some variables that will be used while reading the name. 
            char file_buffer[1024];
            int process_pid;
            char name[256];
            long unsigned int process_user_time, process_kernel_time;

            fgets(file_buffer, sizeof(file_buffer), stats_file); // Reading the entire file for processing.

            // Verifying if the buffer is written in or not. 
            if(file_buffer[0] != '\0'){
                // Parsing and storing the relavent statistics from the file into teh variables.
                sscanf(file_buffer, "%d %511s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu", &process_pid, name, &process_user_time, &process_kernel_time);

                // Changing the variable name to store the name without the parenthesis.
                name[strlen(name) - 1] = '\0';
                memmove(name, name + 1, strlen(name));

            }

            // Storing the relavent features in the data structure.
            processes_store[process_counter].pid = process_pid;
            strncpy(processes_store[process_counter].name, name, sizeof(processes_store[process_counter].name));
            processes_store[process_counter].total_cpu_time = process_user_time + process_kernel_time;
            process_counter++;    

            // Closing the stats file for the PID.
            fclose(stats_file);
        }
    }

    // All the iterations complete and the data about all the processes stored in the processes_store data structure. 

    //Sorting the data structure in descenting order based on the total_cpu_time attribute of the struct that we created for storing the parameters.
    qsort(processes_store, process_counter, sizeof(struct processes_data), compare_processes_comparator);

    // Creating a structure for storing the top 2 cpu consuming processes.
    struct processes_data *return_store = malloc(2 * sizeof(struct processes_data));

    if(return_store == NULL){
        perror("Malloc failed for the return value storing structure.");
        exit(EXIT_FAILURE);
    }
    
    // Finding the top 2 time consuming processes that are present in the server.
    for(int i = 0; i< 2 && i<process_counter; i++){
        return_store[i] = return_store[i];
    }

    return return_store;
}



void *client_handler_function(void *args){

    // Extracting the socket number that we passed in as an argument and then freeing the memory space taken by the args variable.
    int client_socket = *(int *)args;
    free(args);

    // Buffer for storing the data received from the server side.
    char recieving_buffer[2048] = {0};
    
    const char *server_hello = "Hello message from the Server side.";

    //Reading the message sent from the client and storing in the buffer.
    read(client_socket, recieving_buffer, 2048);
    printf("Request received from the client side: %s\n", recieving_buffer);

    // Storing the top 2 CPU consuming processes in the data structure.
    struct processes_data* top_proc_store = find_top_processes();

    // Creating a buffer for storing the string to be sent. 
    char* result_processes = malloc(2048);
    
    // Error handling for the termination.
    if(result_processes == NULL){
        perror("Malloc failed for the output string.");
        exit(EXIT_FAILURE);
    }

    // Creating the output string storing the output to be sent to the client. 
    // int string_length = 0; 
    // string_length += snprintf(result_processes + string_length, 2048-string_length, "Top 2 CPU consuming processes:\n");

    // for(int i = 0; i<2; i++){
    //     string_length += snprintf(result_processes + string_length, 2048 - string_length, "PID: %d, Process Name: %s, Total CPU time: %lu \n", top_proc_store[i].pid, top_proc_store[i].name, top_proc_store[i].total_cpu_time);
    // }


    // // Sending the string generated back to the client.
    // send(client_socket, result_processes, strlen(result_processes), 0);
    // printf("Response message sent to the client. \n");


    char write_buffer1[2048];

    int bytes_written = snprintf(write_buffer1, sizeof(write_buffer1),
    "PID: %d, Process Name: %s, Total CPU time: %lu \n"
    "PID: %d, Process Name: %s, Total CPU time: %lu \n",
    top_proc_store[0].pid, top_proc_store[0].name, top_proc_store[0].total_cpu_time,
    top_proc_store[1].pid, top_proc_store[1].name, top_proc_store[1].total_cpu_time);

    send(client_socket, write_buffer1, bytes_written, 0);


    // free(top_proc_store);
    // free(result_processes);
    close(client_socket);
    // Gracefully terminating the thread.
    pthread_exit(NULL);
}



int main(){

    //Declaring the socket
    int server_fd;

    // Initialising the socket and checking-verifying if the process was a success or not.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Failed to create a socket");
        exit(EXIT_FAILURE);
    }

    // Initialising a data structure to represent the IP addr and port number of the socket interface.
    struct sockaddr_in address;

    // Storing the size of the address variable in the given variable.
    int addr_len = sizeof(address);

    // Specifying the server side parameters that the server will accept for the incoming connections. 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

	
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
	
    // Binding the socket that we created to listen to the specified IP address and the specified host.
    if (bind(server_fd, (struct sockaddr *)&address, addr_len) < 0) {
        perror("Binding of socket to IP and Port failed. Terminating the script.");
        exit(EXIT_FAILURE);
    }

    // Listning for the incoming connection at the socket which we just binded.
    if (listen(server_fd,5) < 0) {
        perror("Listening for the connections failed. Terminating the script.");
        exit(EXIT_FAILURE);
    }


    // Creating an infinite loop for continuously listening for the incoming requests for the connection we just while listening for the connections.
    while(1){

        // Creating a new socket which will be returned by the accept method call and is used for showing the dedicated connection with that specific client. 
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len)) < 0) {
            perror("Accept connection failed. Failed to make socket for the specific connection. Terminating Script.");
            exit(EXIT_FAILURE);
        }

        // Allocating memory for the new socket as we will need to pass it as a pointer to the pthread for multiple connection. 
        int *new_socket_ptr = malloc(sizeof(int));
        *new_socket_ptr = new_socket;

        // Declaring a thread ID for the thread.
        pthread_t thread_id;

        // Creating a new thread for multi threading the connections.
        pthread_create(&thread_id, NULL, client_handler_function, new_socket_ptr);
        
        // Detach the thread
        pthread_detach(thread_id); 
    }

    // Closing the server socket before terminating the script.
    close(server_fd);

    return 0;
}
