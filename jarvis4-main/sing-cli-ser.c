
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server, client;
    char buffer[1024];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(server_socket,
         (struct sockaddr *)&server,
         sizeof(server));

    listen(server_socket, 5);

    printf("Server listening...\n");
    fflush(stdout);

    socklen_t len = sizeof(client);

    client_socket = accept(
        server_socket,
        (struct sockaddr *)&client,
        &len
    );

    printf("Client connected\n");

    while (1) {

        // Receive message from client
        memset(buffer, 0, sizeof(buffer));

        recv(client_socket, buffer, sizeof(buffer), 0);

        printf("Client: %s\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        // Send message to client
        printf("Server: ");
        fgets(buffer, sizeof(buffer), stdin);


        send(client_socket,
             buffer,
             strlen(buffer) + 1,
             0);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }

    close(client_socket);
    close(server_socket);

    return 0;
}