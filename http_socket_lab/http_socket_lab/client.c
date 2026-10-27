#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8000
#define BUFFER_SIZE 4096

static double now_seconds(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (double)t.tv_sec + (double)t.tv_nsec / 1000000000.0;
}

int main(void)
{
    int sock, bytes = 0, header_done = 0;
    struct sockaddr_in server;
    char request[1024], buffer[BUFFER_SIZE];
    char header_buffer[8192];
    size_t header_len = 0;
    const char *path = "/index.html";
    const char *filename = "downloaded_index.html";
    double start, first = 0.0, end;
    FILE *fp;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP address.\n");
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        fprintf(stderr, "Start the HTTP server first at %s:%d.\n",
                SERVER_IP, SERVER_PORT);
        close(sock);
        return EXIT_FAILURE;
    }

    int req_len = snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\nHost: localhost\r\nConnection: close\r\n\r\n", path);
    if (req_len < 0 || (size_t)req_len >= sizeof(request)) {
        fprintf(stderr, "Request is too long.\n");
        close(sock);
        return EXIT_FAILURE;
    }

    start = now_seconds();
    size_t sent = 0;
    while (sent < (size_t)req_len) {
        ssize_t n = send(sock, request + sent, (size_t)req_len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("send");
            close(sock);
            return EXIT_FAILURE;
        }
        sent += (size_t)n;
    }

    fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        close(sock);
        return EXIT_FAILURE;
    }

    while (1) {
        ssize_t n = recv(sock, buffer, sizeof(buffer), 0);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            fclose(fp);
            close(sock);
            return EXIT_FAILURE;
        }

        if (first == 0.0)
            first = now_seconds();

        size_t body_start = 0;
        if (!header_done) {
            size_t i;
            for (i = 0; i < (size_t)n; i++) {
                if (header_len >= sizeof(header_buffer) - 1) {
                    fprintf(stderr, "HTTP header exceeds buffer size.\n");
                    fclose(fp);
                    close(sock);
                    remove(filename);
                    return EXIT_FAILURE;
                }

                header_buffer[header_len++] = buffer[i];
                header_buffer[header_len] = '\0';

                if (header_len >= 4 &&
                    memcmp(header_buffer + header_len - 4, "\r\n\r\n", 4) == 0) {
                    header_done = 1;
                    body_start = i + 1;
                    printf("HTTP response received\n");
                    printf("Status: %.100s\n", header_buffer);
                    printf("Response time: %.3f ms\n", (first - start) * 1000.0);
                    break;
                }
            }

            if (!header_done)
                continue;
        }

        if (header_done && body_start < (size_t)n) {
            size_t body_len = (size_t)n - body_start;
            if (fwrite(buffer + body_start, 1, body_len, fp) != body_len) {
                perror("fwrite");
                fclose(fp);
                close(sock);
                remove(filename);
                return EXIT_FAILURE;
            }
            bytes += (int)body_len;
        }
    }

    end = now_seconds();
    fclose(fp);
    close(sock);

    if (!header_done) {
        fprintf(stderr, "No complete HTTP header was received.\n");
        remove(filename);
        return EXIT_FAILURE;
    }

    printf("Downloaded file: %s\n", filename);
    printf("Data size: %d bytes\n", bytes);
    printf("Download time: %.3f seconds\n", end - first);
    printf("Total time: %.3f seconds\n", end - start);

    if (end > first)
        printf("Throughput: %.2f KB/s\n", bytes / 1024.0 / (end - first));
    else
        printf("Throughput: N/A (download completed too quickly to measure)\n");

    return EXIT_SUCCESS;
}
