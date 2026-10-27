
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);
    char frame[1000], gen[] = "10011", temp[1000];
    int i, j, n, error = 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    listen(server_socket, 5);

    printf("Receiver waiting...\n");

    client_socket = accept(server_socket, (struct sockaddr *)&client, &len);

    n = recv(client_socket, frame, sizeof(frame) - 1, 0);
    frame[n] = '\0';

    printf("Received Frame: %s\n", frame);

    // Introduce one bit error
    frame[0] = (frame[0] == '0') ? '1' : '0';

    printf("Frame after error: %s\n", frame);

    strcpy(temp, frame);
    int size = strlen(temp);

    // CRC verification
    for (i = 0; i <= size - 5; i++)
    {
        if (temp[i] == '1')
        {
            for (j = 0; j < 5; j++)
                temp[i + j] =
                    (temp[i + j] == gen[j]) ? '0' : '1';
        }
    }

    printf("Remainder: ");

    for (i = size - 4; i < size; i++)
    {
        printf("%c", temp[i]);

        if (temp[i] == '1')
            error = 1;
    }

    if (error)
        printf("\nFrame contains errors\n");
    else
        printf("\nFrame is error-free\n");

    close(client_socket);
    close(server_socket);

    return 0;
}
