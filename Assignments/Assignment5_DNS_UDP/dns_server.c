/* UDP DNS simulator - server.
   Build: gcc dns_server.c -o dns_server
   Usage: ./dns_server [loss_%] [delay_ms] [corrupt_%]    e.g. ./dns_server 30 100 10  (to test timeout/CRC) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "crc32.h"
#define PORT 5353
#define TTL 30
typedef struct { const char *name, *ip; } Rec;
static Rec table[] = {   /* DNS database: hostname -> IP */
    {"google.com", "142.250.195.46"}, {"example.com", "93.184.216.34"}, {"ssn.edu.in", "103.21.58.10"},
    {"github.com", "140.82.112.3"},   {"anna.univ.edu", "10.0.0.25"},   {"localhost", "127.0.0.1"},
    {"wikipedia.org", "208.80.154.224"}, {"youtube.com", "142.250.77.110"}
};
#define N (int)(sizeof table / sizeof table[0])

static int valid_name(const char *s) {
    size_t n = strlen(s);
    if (n < 1 || n > 253 || !strchr(s, '.') || s[0] == '.' || s[n-1] == '.' || s[0] == '-') return 0;
    for (size_t i = 0; i < n; i++) if (!isalnum((unsigned char)s[i]) && s[i] != '.' && s[i] != '-') return 0;
    return 1;
}
int main(int argc, char **argv) {
    int loss = argc > 1 ? atoi(argv[1]) : 0, delay = argc > 2 ? atoi(argv[2]) : 0, corrupt = argc > 3 ? atoi(argv[3]) : 0;
    srand(time(NULL));
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = {0}, c; socklen_t cl = sizeof c;
    a.sin_family = AF_INET; a.sin_addr.s_addr = INADDR_ANY; a.sin_port = htons(PORT);
    if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) { perror("bind"); return 1; }
    printf("DNS server on UDP %d (loss=%d%% delay=%dms corrupt=%d%%)\n", PORT, loss, delay, corrupt);
    for (;;) {
        char buf[512], payload[256], out[300];
        ssize_t n = recvfrom(s, buf, sizeof buf - 1, 0, (struct sockaddr *)&c, &cl);
        if (n <= 0) continue; buf[n] = 0;
        if (rand() % 100 < loss) { printf("[sim] query dropped\n"); continue; }       /* timeout simulation */
        if (!crc_unpack(buf)) { printf("[err] CRC mismatch, ignoring\n"); continue; }
        if (strncmp(buf, "QUERY|", 6) != 0 || !valid_name(buf + 6)) snprintf(payload, sizeof payload, "ERROR|INVALID_FORMAT");
        else {
            char name[256]; snprintf(name, sizeof name, "%s", buf + 6);
            for (char *p = name; *p; p++) *p = tolower((unsigned char)*p);
            const char *ip = NULL; for (int i = 0; i < N; i++) if (!strcmp(name, table[i].name)) ip = table[i].ip;
            if (ip) snprintf(payload, sizeof payload, "OK|%s|%d", ip, TTL); else snprintf(payload, sizeof payload, "NXDOMAIN");
        }
        printf("%s:%d  %s  ->  %s\n", inet_ntoa(c.sin_addr), ntohs(c.sin_port), buf, payload);
        if (delay) usleep(delay * 1000);
        crc_pack(out, sizeof out, payload);
        if (rand() % 100 < corrupt) { out[0] ^= 1; printf("[sim] reply corrupted\n"); }
        sendto(s, out, strlen(out), 0, (struct sockaddr *)&c, cl);
    }
}
