#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>  // Include errno for error handling

// Define the data structure to hold process information
struct processes_data {
    int pid;
    char name[512];
    long unsigned int total_cpu_time; 
};

// Comparator for sorting processes by CPU time (descending)
int compare_processes_comparator(const void *a, const void *b) {
    struct processes_data *procA = (struct processes_data *)a;
    struct processes_data *procB = (struct processes_data *)b;
    return (procB->total_cpu_time - procA->total_cpu_time);
}

// Function to find top 2 CPU consuming processes
struct processes_data* find_top_processes() {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Unable to open /proc directory");
        exit(EXIT_FAILURE);
    }

    struct processes_data processes_store[2048];
    int process_counter = 0;
    struct dirent *entry;

    while ((entry = readdir(proc_dir)) != NULL) {
        if (isdigit(*entry->d_name)) {
            char stat_file_path[512];
            snprintf(stat_file_path, sizeof(stat_file_path), "/proc/%s/stat", entry->d_name);

            FILE *stats_file = fopen(stat_file_path, "r");
            if (stats_file == NULL) {
                continue;
            }

            char file_buffer[1024];
            int process_pid;
            char name[256];
            long unsigned int process_user_time, process_kernel_time;

            fgets(file_buffer, sizeof(file_buffer), stats_file);
            if (file_buffer[0] != '\0') {
                sscanf(file_buffer, "%d %511s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %lu", &process_pid, name, &process_user_time, &process_kernel_time);
                name[strlen(name) - 1] = '\0';
                memmove(name, name + 1, strlen(name));
            }

            processes_store[process_counter].pid = process_pid;
            strncpy(processes_store[process_counter].name, name, sizeof(processes_store[process_counter].name));
            processes_store[process_counter].total_cpu_time = process_user_time + process_kernel_time;
            process_counter++;

            fclose(stats_file);
        }
    }

    qsort(processes_store, process_counter, sizeof(struct processes_data), compare_processes_comparator);

    struct processes_data *return_store = malloc(2 * sizeof(struct processes_data));
    if (return_store == NULL) {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 2 && i < process_counter; i++) {
        return_store[i] = processes_store[i];
    }

    closedir(proc_dir);
    return return_store;
}

// Function to handle client requests
void handle_client_request(int client_socket) {
    char receiving_buffer[2048] = {0};
    read(client_socket, receiving_buffer, 2048);
    printf("Request received from client: %s\n", receiving_buffer);

    struct processes_data *top_proc_store = find_top_processes();

    char write_buffer1[2048];
    int bytes_written = snprintf(write_buffer1, sizeof(write_buffer1),
                                 "PID: %d, Process Name: %s, Total CPU time: %lu\n"
                                 "PID: %d, Process Name: %s, Total CPU time: %lu\n",
                                 top_proc_store[0].pid, top_proc_store[0].name, top_proc_store[0].total_cpu_time,
                                 top_proc_store[1].pid, top_proc_store[1].name, top_proc_store[1].total_cpu_time);

    send(client_socket, write_buffer1, bytes_written, 0);
    free(top_proc_store);
    close(client_socket);
}

int main() {
    int server_fd, max_sd, activity, new_socket, client_socket[30], max_clients = 30;
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    fd_set readfds;

    // Initialize all client_socket[] to 0 so that they're not checked initially
    for (int i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, addr_len) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < max_clients; i++) {
            int sd = client_socket[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for an activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error");
        }

        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            // Add new socket to array of sockets
            for (int i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        // Else it's some IO operation on a client socket
        for (int i = 0; i < max_clients; i++) {
            int sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) {
                handle_client_request(sd);
                client_socket[i] = 0;
            }
        }
    }

    close(server_fd);
    return 0;
}

