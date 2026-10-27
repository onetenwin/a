
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

int s;
struct sockaddr_in server;

void *receive_msg(void *arg) {
    char b[1024];
    int n;
    while ((n = recvfrom(s, b, sizeof(b)-1, 0, NULL, NULL)) > 0) {
        b[n] = '\0';
        printf("\n%s", b);
        fflush(stdout);
    }
    return NULL;
}

int main() {
    s = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    pthread_t t;
    pthread_create(&t, NULL, receive_msg, NULL);

    char msg[1024];
    while (fgets(msg, sizeof(msg), stdin))
        sendto(s, msg, strlen(msg), 0,
               (void *)&server, sizeof(server));

    close(s);
    return 0;
}
