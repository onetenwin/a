
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

int main()
{
    int sock, n, bytes = 0;
    struct sockaddr_in server;
    char request[1024], buffer[4096];
    char path[] = "/index.html";
    char filename[] = "index.html";
    char *p;
    int header_done = 0;
    double start, first, end;
    struct timespec t;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0) {
        perror("connect");
        return 1;
    }

    sprintf(request,
            "GET %s HTTP/1.0\r\nHost: localhost\r\n\r\n", path);

    clock_gettime(CLOCK_MONOTONIC, &t);
    start = t.tv_sec + t.tv_nsec / 1000000000.0;

    send(sock, request, strlen(request), 0);

    FILE *fp = fopen(filename, "wb");

    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        if (!header_done) {
            p = strstr(buffer, "\r\n\r\n");

            if (p != NULL) {
                int pos = p - buffer + 4;

                clock_gettime(CLOCK_MONOTONIC, &t);
                first = t.tv_sec + t.tv_nsec / 1000000000.0;

                printf("HTTP response received\n");
                printf("Response time: %.3f ms\n",
                       (first - start) * 1000);

                fwrite(p + 4, 1, n - pos, fp);
                bytes += n - pos;
                header_done = 1;
            }
        }
        else {
            fwrite(buffer, 1, n, fp);
            bytes += n;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t);
    end = t.tv_sec + t.tv_nsec / 1000000000.0;

    fclose(fp);
    close(sock);

    printf("Downloaded file: %s\n", filename);
    printf("Data size: %d bytes\n", bytes);
    printf("Download time: %.3f seconds\n", end - first);
    printf("Total time: %.3f seconds\n", end - start);

    if (end > first)
        printf("Throughput: %.2f KB/s\n",
               bytes / 1024.0 / (end - first));

    return 0;
}
