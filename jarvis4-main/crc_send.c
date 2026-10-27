
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int client_socket, data[] = {10, 20, 30};
    int i, j, k = 0, n = 3;
    struct sockaddr_in server;
    char frame[1000], temp[1000], gen[] = "10011";
    char rem[5];

    // Convert integers to binary
    for (i = 0; i < n; i++)
    {
        for (j = 7; j >= 0; j--)
            frame[k++] = data[i] % 2 + '0',
            data[i] = data[i] / 2;
    }

    // Correct the conversion by using original values
    int values[] = {10, 20, 30};
    k = 0;

    for (i = 0; i < n; i++)
    {
        for (j = 7; j >= 0; j--)
            frame[k++] = ((values[i] >> j) & 1) + '0';
    }

    frame[k] = '\0';

    printf("Binary Data: %s\n", frame);

    strcpy(temp, frame);
    int len = strlen(frame);

    for (i = 0; i < 4; i++)
        temp[len + i] = '0';

    temp[len + 4] = '\0';

    // CRC division using XOR
    for (i = 0; i < len; i++)
    {
        if (temp[i] == '1')
        {
            for (j = 0; j < 5; j++)
                temp[i + j] =
                    (temp[i + j] == gen[j]) ? '0' : '1';
        }
    }

    for (i = 0; i < 4; i++)
        rem[i] = temp[len + i];

    rem[4] = '\0';

    printf("CRC: %s\n", rem);

    strcat(frame, rem);

    printf("Transmitted Frame: %s\n", frame);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_socket, (struct sockaddr *)&server, sizeof(server));

    send(client_socket, frame, strlen(frame) + 1, 0);

    printf("Frame sent to receiver\n");

    close(client_socket);

    return 0;
}
