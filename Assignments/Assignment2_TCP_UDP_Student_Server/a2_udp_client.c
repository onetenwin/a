/* Assignment 2(b) - UDP client with 3 s timeout. Usage: ./a2_udp_client [server_ip] */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "crc32.h"
#define PORT 9090
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

int main(int argc, char **argv) {
    const char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv = {3, 0}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(PORT); inet_pton(AF_INET, ip, &a.sin_addr);
    char roll[64], out[128], buf[512];
    printf("Enter Roll Number: "); if (!fgets(roll, sizeof roll, stdin)) return 1;
    roll[strcspn(roll, "\r\n")] = 0;
    crc_pack(out, sizeof out, roll);
    double t0 = now();
    sendto(s, out, strlen(out), 0, (struct sockaddr *)&a, sizeof a);
    ssize_t n = recvfrom(s, buf, sizeof buf - 1, 0, NULL, NULL);
    double t1 = now();
    if (n < 0) { printf(errno == EAGAIN || errno == EWOULDBLOCK ? "Timeout: no reply (datagram lost?)\n" : "recv error\n"); close(s); return 1; }
    buf[n] = 0;
    if (!crc_unpack(buf)) printf("Reply corrupted (CRC mismatch)\n");
    else printf("Response: %s\nRTT: %.3f ms\n", buf, (t1 - t0) * 1e3);
    close(s); return 0;
}
