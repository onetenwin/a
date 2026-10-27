
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

double now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main() {
    char path[200], req[500], buf[4096], head[8192];
    int s, n, used = 0, found = 0;
    char *p;
    long size = 0;
    double start, first = 0, finish;

    printf("Enter file path: ");
    scanf("%199s", path);

    s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(8000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (s < 0 || connect(s, (void *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        if (s >= 0) close(s);
        return 1;
    }

    snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n",
        path);

    FILE *f = fopen("downloads/file", "wb");
    if (!f) {
        perror("Open file");
        close(s);
        return 1;
    }

    start = now();
    if (send(s, req, strlen(req), 0) < 0) {
        perror("Send failed");
        fclose(f);
        close(s);
        return 1;
    }

    while ((n = recv(s, buf, sizeof(buf), 0)) > 0) {
        if (!found) {
            if (used == 0) first = now();
            if (used + n >= sizeof(head)) break;

            memcpy(head + used, buf, n);
            used += n;
            head[used] = '\0';

            p = strstr(head, "\r\n\r\n");
            if (p) {
                int h = p - head + 4;
                found = 1;
                printf("%.*s", (int)(strstr(head, "\r\n") - head), head);
                size = used - h;
                fwrite(head + h, 1, size, f);
            }
        } else {
            fwrite(buf, 1, n, f);
            size += n;
        }
    }

    finish = now();
    fclose(f);
    close(s);

    if (!found) {
        printf("Invalid HTTP response\n");
        return 1;
    }

    printf("\nSaved to downloads/file\n");
    printf("Response time: %.6f s\n", first - start);
    printf("Download time: %.6f s\n", finish - first);
    printf("Data size: %ld bytes\n", size);
    if (finish > first)
        printf("Throughput: %.2f bytes/s\n", size / (finish - first));

    return 0;
}
