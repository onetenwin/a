#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server, client;
    char buffer[1024];

    signal(SIGCHLD, SIG_IGN);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    listen(server_socket, 5);

    printf("Server listening...\n");

    while (1)
    {
        socklen_t len = sizeof(client);

        client_socket = accept(server_socket,
                         (struct sockaddr *)&client, &len);

        printf("Client connected\n");

        if (fork() == 0)
        {
            close(server_socket);

            while (1)
            {
                memset(buffer, 0, sizeof(buffer));

                int n = recv(client_socket, buffer,
                             sizeof(buffer) - 1, 0);

                if (n <= 0)
                    break;

                buffer[n] = '\0';
                buffer[strcspn(buffer, "\n")] = '\0';

                printf("Client: %s\n", buffer);

                if (strcmp(buffer, "exit") == 0)
                    break;

                printf("Server: ");
                fflush(stdout);

                if (fgets(buffer, sizeof(buffer), stdin) == NULL)
                    break;

                buffer[strcspn(buffer, "\n")] = '\0';

                send(client_socket, buffer, strlen(buffer), 0);

                if (strcmp(buffer, "exit") == 0)
                    break;
            }

            close(client_socket);
            return 0;
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}