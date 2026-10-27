
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>

int s;

void *receive_msg(void *arg) {
    char b[1024];
    int n;
    while ((n = recv(s, b, sizeof(b)-1, 0)) > 0) {
        b[n] = '\0';
        printf("\n%s", b);
        fflush(stdout);
    }
    return NULL;
}

int main() {
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(s, (void *)&a, sizeof(a)) < 0) {
        perror("connect");
        return 1;
    }

    pthread_t t;
    pthread_create(&t, NULL, receive_msg, NULL);

    char msg[1024];
    while (fgets(msg, sizeof(msg), stdin))
        send(s, msg, strlen(msg), 0);

    shutdown(s, SHUT_RDWR);
    close(s);
    return 0;
}
