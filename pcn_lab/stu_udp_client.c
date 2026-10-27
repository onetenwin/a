
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

int main() {
    int s, n;
    char roll[20], result[300];
    struct sockaddr_in server = {0};
    socklen_t len = sizeof(server);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    struct timeval tv = {3, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Enter Roll Number: ");
    scanf("%19s", roll);

    sendto(s, roll, strlen(roll), 0,
           (struct sockaddr *)&server, len);

    n = recvfrom(s, result, sizeof(result) - 1, 0, NULL, NULL);

    if (n < 0) {
        printf("Server timeout\n");
    } else {
        result[n] = '\0';
        printf("%s\n", result);
    }

    close(s);
    return 0;
}
