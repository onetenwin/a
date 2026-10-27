#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int client_socket;
    struct sockaddr_in server;
    char buffer[1024];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Connecting to server...\n");

    connect(client_socket,
            (struct sockaddr *)&server,
            sizeof(server));

    printf("Connected to server\n");

    while (1) {

        // Send message to server
        printf("Client: ");
        fgets(buffer, sizeof(buffer), stdin);


        send(client_socket,
             buffer,
             strlen(buffer) + 1,
             0);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        // Receive message from server
        memset(buffer, 0, sizeof(buffer));

        recv(client_socket,
             buffer,
             sizeof(buffer),
             0);

        printf("Server: %s\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }

    close(client_socket);

    return 0;
}