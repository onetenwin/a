
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int s, n, count = 0;
    char msg[1024];
    struct sockaddr_in server, clients[10], from;
    socklen_t len = sizeof(from);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(9001);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (void *)&server, sizeof(server));
    printf("UDP chat server running on port 9001\n");

    while (1) {
        n = recvfrom(s, msg, sizeof(msg), 0,
                     (void *)&from, &len);
        if (n < 0) continue;

        int found = 0;
        for (int i = 0; i < count; i++)
            if (clients[i].sin_port == from.sin_port &&
                clients[i].sin_addr.s_addr == from.sin_addr.s_addr)
                found = 1;

        if (!found && count < 10)
            clients[count++] = from;

        for (int i = 0; i < count; i++)
            sendto(s, msg, n, 0, (void *)&clients[i], len);
    }
}
