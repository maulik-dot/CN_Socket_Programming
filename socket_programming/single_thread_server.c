#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>

// Define the data structure to hold process information
struct processes_data {
    int pid;
    char name[512];
    long unsigned int total_cpu_time;
};

// Custom comparator to sort processes in reverse order by CPU time
int compare_processes_comparator(const void *a, const void *b) {
    struct processes_data *procA = (struct processes_data *)a;
    struct processes_data *procB = (struct processes_data *)b;
    return (procB->total_cpu_time - procA->total_cpu_time);  
}

// Function to find the top 2 CPU-consuming processes
struct processes_data* find_top_processes() {
    DIR *proc_dir = opendir("/proc");

    if (proc_dir == NULL) {
        perror("Unable to open the /proc directory");
        exit(EXIT_FAILURE);
    }

    struct processes_data processes_store[2048]; // Assuming max of 2048 processes
    int process_counter = 0; // Process count

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        // Check if the directory name is a number (i.e., a PID)
        if (isdigit(*entry->d_name)) {
            char stat_file_path[512];
            snprintf(stat_file_path, sizeof(stat_file_path), "/proc/%s/stat", entry->d_name);

            FILE *stats_file = fopen(stat_file_path, "r");
            if (stats_file == NULL) {
                continue; // Skip if the file cannot be opened
            }

            char file_buffer[1024];
            int process_pid;
            char name[256];
            long unsigned int process_user_time, process_kernel_time;

            fgets(file_buffer, sizeof(file_buffer), stats_file);
            if (file_buffer[0] != '\0') {
                sscanf(file_buffer, "%d %511s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu",
                       &process_pid, name, &process_user_time, &process_kernel_time);
                name[strlen(name) - 1] = '\0'; // Remove last parenthesis
                memmove(name, name + 1, strlen(name)); // Remove first parenthesis
            }

            // Store process information
            processes_store[process_counter].pid = process_pid;
            strncpy(processes_store[process_counter].name, name, sizeof(processes_store[process_counter].name));
            processes_store[process_counter].total_cpu_time = process_user_time + process_kernel_time;
            process_counter++;

            fclose(stats_file);
        }
    }
    closedir(proc_dir);

    // Sort processes in descending order of CPU time
    qsort(processes_store, process_counter, sizeof(struct processes_data), compare_processes_comparator);

    // Return the top 2 CPU-consuming processes
    struct processes_data *return_store = malloc(2 * sizeof(struct processes_data));
    if (return_store == NULL) {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 2 && i < process_counter; i++) {
        return_store[i] = processes_store[i]; // Copy top 2 processes
    }

    return return_store;
}

int main() {
    int server_fd, new_socket;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Specify the server's address and port
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the specified IP address and port
    if (bind(server_fd, (struct sockaddr *)&address, addr_len) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);// Continue to the next connection
        }

        char receiving_buffer[2048] = {0};
        read(new_socket, receiving_buffer, 2048);
        printf("Request received: %s\n", receiving_buffer);

        // Find the top 2 CPU-consuming processes
        struct processes_data *top_processes = find_top_processes();

        // Prepare response
        char response[1024];
        snprintf(response, sizeof(response), "Top 2 processes:\n1. %s (PID: %d) - CPU Time: %lu\n2. %s (PID: %d) - CPU Time: %lu\n",
                 top_processes[0].name, top_processes[0].pid, top_processes[0].total_cpu_time,
                 top_processes[1].name, top_processes[1].pid, top_processes[1].total_cpu_time);

        // Send response to the client
        send(new_socket, response, strlen(response), 0);
        printf("Response sent to client.\n");

        // Free memory and close the connection
        free(top_processes);
        close(new_socket);
    

    // Close the server socket (this line will never be reached in this code)
    close(server_fd);
    return 0;
}

