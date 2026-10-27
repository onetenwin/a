
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server = {0};
    char roll[20], result[500];
    int n;

    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    connect(s, (struct sockaddr *)&server, sizeof(server));

    printf("Enter Roll Number: ");
    scanf("%19s", roll);
    send(s, roll, strlen(roll) + 1, 0);

    while ((n = recv(s, result, sizeof(result) - 1, 0)) > 0) {
        result[n] = '\0';
        printf("%s", result);
    }

    close(s);
    return 0;
}
