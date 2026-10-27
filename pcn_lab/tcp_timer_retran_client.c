
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    connect(s, (void *)&a, sizeof(a));

    struct timeval tv = {2, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char ack[10];
    for (int i = 1; i <= 3; i++) {
        printf("Sending request: Attempt %d\n", i);
        send(s, "HELLO", 5, 0);

        int n = recv(s, ack, sizeof(ack), 0);
        if (n > 0) {
            printf("Received ACK\n");
            break;
        }
        printf("Timeout! Retransmitting...\n");
    }

    close(s);
    return 0;
}
