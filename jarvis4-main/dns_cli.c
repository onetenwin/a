
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

int main()
{
    int client_socket, i, found, bytes;
    struct sockaddr_in server;
    char buffer[1024], reply[1024];
    char cache_domain[10][100], cache_ip[10][100];
    int count = 0;
    socklen_t len;

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Set timeout to 3 seconds
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));

    while (1)
    {
        printf("\nEnter domain (exit to stop): ");

        memset(buffer, 0, sizeof(buffer));
        scanf("%99s", buffer);

        if (strcmp(buffer, "exit") == 0)
            break;

        if (strchr(buffer, '.') == NULL)
        {
            printf("Invalid format\n");
            continue;
        }

        found = 0;

        // Check cache
        for (i = 0; i < count; i++)
        {
            if (strcmp(buffer, cache_domain[i]) == 0)
            {
                printf("IP Address: %s (Cached)\n", cache_ip[i]);
                found = 1;
                break;
            }
        }

        if (found)
            continue;

        // Send DNS query
        sendto(client_socket, buffer, strlen(buffer) + 1, 0,
               (struct sockaddr *)&server, sizeof(server));

        memset(reply, 0, sizeof(reply));
        len = sizeof(server);

        bytes = recvfrom(client_socket, reply, sizeof(reply) - 1, 0,
                         (struct sockaddr *)&server, &len);

        if (bytes < 0)
        {
            printf("DNS request timed out\n");
            continue;
        }

        reply[bytes] = '\0';

        if (strcmp(reply, "NXDOMAIN") == 0)
        {
            printf("Domain not found (NXDOMAIN)\n");
        }
        else if (strcmp(reply, "INVALID FORMAT") == 0)
        {
            printf("Invalid format\n");
        }
        else
        {
            printf("IP Address: %s\n", reply);

            // Store successful lookup in cache
            if (count < 10)
            {
                strcpy(cache_domain[count], buffer);
                strcpy(cache_ip[count], reply);
                count++;
            }
        }
    }

    close(client_socket);
    return 0;
}
