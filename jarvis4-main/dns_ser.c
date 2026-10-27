
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int server_socket, i, found;
    struct sockaddr_in server, client;
    char buffer[1024], reply[1024];
    socklen_t len;

    char *domain[] = {"google.com", "youtube.com", "example.com"};
    char *ip[] = {"142.250.183.14", "142.250.72.14", "93.184.216.34"};

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(server_socket, (struct sockaddr *)&server, sizeof(server));

    printf("DNS Server is waiting...\n");

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        memset(reply, 0, sizeof(reply));

        len = sizeof(client);

        recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0,
                 (struct sockaddr *)&client, &len);

        printf("Domain: %s\n", buffer);

        if (strcmp(buffer, "timeout.test") == 0)
            continue;

        if (strchr(buffer, '.') == NULL)
        {
            strcpy(reply, "INVALID FORMAT");
        }
        else
        {
            found = 0;

            for (i = 0; i < 3; i++)
            {
                if (strcmp(buffer, domain[i]) == 0)
                {
                    strcpy(reply, ip[i]);
                    found = 1;
                    break;
                }
            }

            if (found == 0)
                strcpy(reply, "NXDOMAIN");
        }

        sendto(server_socket, reply, strlen(reply) + 1, 0,
               (struct sockaddr *)&client, len);
    }

    close(server_socket);
    return 0;
}
