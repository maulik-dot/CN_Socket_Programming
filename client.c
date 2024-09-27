#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // For inet_addr()
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // For write() and close()

#define DESTPORT 3500

int main()
{
    struct sockaddr_in dest_addr; // IPv4 socket address structure

    // Creation of socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    printf("%u\n", htons(DESTPORT));

    // Assigning Socket Parameters
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DESTPORT); // This field holds the port number, which identifies the specific process or service on the machine to which you want to send data.
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(dest_addr.sin_zero), '\0', 8); // Zero the rest of the struct (correctly initialize sin_zero)

    if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("Connection failed");
        return 1;
    }

    while (1)
    {
        char msg[100];
        printf("Enter your message: ");
        fgets(msg, sizeof(msg), stdin); // Safe alternative to gets()

        // Remove newline from fgets input if present
        msg[strcspn(msg, "\n")] = 0;

        int w = write(sockfd, msg, strlen(msg));
        if (w < 0)
        {
            perror("Write failed");
        }
    }

    close(sockfd);
    printf("Client shutting down...\n");

    return 0;
}
