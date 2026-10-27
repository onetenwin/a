/* Assignment 2(a) - TCP client. Usage: ./a2_tcp_client [server_ip] */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 8080
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

int main(int argc, char **argv) {
    const char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &a.sin_addr);
    double t0 = now();
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { perror("connect"); return 1; }
    double t_conn = now();
    char roll[64], buf[1024]; printf("Enter Roll Number: ");
    if (!fgets(roll, sizeof roll, stdin)) return 1;
    roll[strcspn(roll, "\r\n")] = 0;
    double t1 = now();
    send(s, roll, strlen(roll), 0);
    ssize_t n, total = 0; printf("\n--- Server Response ---\n");
    while ((n = recv(s, buf, sizeof buf - 1, 0)) > 0) { buf[n] = 0; printf("%s", buf); total += n; }
    double t2 = now();
    close(s);
    printf("-----------------------\nConnect time: %.3f ms | RTT (req->resp): %.3f ms | bytes: %zd\n",
           (t_conn - t0) * 1e3, (t2 - t1) * 1e3, total);
    return 0;
}
