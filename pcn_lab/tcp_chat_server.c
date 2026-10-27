
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>

int clients[10], count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *handle(void *arg) {
    int c = *(int *)arg, n;
    char msg[1024];
    free(arg);

    while ((n = recv(c, msg, sizeof(msg), 0)) > 0) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < count; i++)
            if (clients[i] != c)
                send(clients[i], msg, n, 0);
        pthread_mutex_unlock(&lock);
    }

    pthread_mutex_lock(&lock);
    for (int i = 0; i < count; i++)
        if (clients[i] == c) {
            clients[i] = clients[--count];
            break;
        }
    pthread_mutex_unlock(&lock);

    close(c);
    return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (void *)&a, sizeof(a));
    listen(s, 10);
    printf("TCP chat server running on port 9000\n");

    while (1) {
        int c = accept(s, NULL, NULL);
        if (c < 0) continue;

        pthread_mutex_lock(&lock);
        if (count == 10) {
            pthread_mutex_unlock(&lock);
            close(c);
            continue;
        }
        clients[count++] = c;
        pthread_mutex_unlock(&lock);

        int *p = malloc(sizeof(int));
        *p = c;
        pthread_t t;
        pthread_create(&t, NULL, handle, p);
        pthread_detach(t);
    }
}
