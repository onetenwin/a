/* UDP DNS simulator - client with cache, timeout/retry, CRC check and statistics.
   Build: gcc dns_client.c -o dns_client
   Usage: ./dns_client [server_ip]      commands: <domain> | stats | cache | quit */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "crc32.h"
#define PORT 5353
#define TIMEOUT_S 2
#define MAX_RETRY 3
#define CACHE_N 8
typedef struct { char name[256], ip[64]; time_t exp; int used; } Cache;
static Cache cache[CACHE_N]; static int nxt = 0;
static long sent = 0, recvd = 0, corrupt = 0, timeouts = 0, hits = 0, queries = 0, bytes_out = 0, bytes_in = 0;
static double rtt_sum = 0, rtt_min = 1e9, rtt_max = 0;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

static Cache *cache_find(const char *n) {
    for (int i = 0; i < CACHE_N; i++) if (cache[i].used && !strcmp(cache[i].name, n)) {
        if (cache[i].exp > time(NULL)) return &cache[i];
        cache[i].used = 0; return NULL;            /* expired */
    } return NULL;
}
static void cache_put(const char *n, const char *ip, int ttl) {
    int slot = -1; for (int i = 0; i < CACHE_N; i++) if (cache[i].used && !strcmp(cache[i].name, n)) slot = i;
    if (slot < 0) { slot = nxt; nxt = (nxt + 1) % CACHE_N; }   /* FIFO replacement */
    snprintf(cache[slot].name, 256, "%s", n); snprintf(cache[slot].ip, 64, "%s", ip);
    cache[slot].exp = time(NULL) + ttl; cache[slot].used = 1;
}
static void print_stats(void) {
    printf("\n===== Statistics =====\nQueries: %ld  Cache hits: %ld (%.1f%%)\n", queries, hits, queries ? 100.0 * hits / queries : 0);
    printf("UDP datagrams sent: %ld  replies OK: %ld  timeouts: %ld  corrupted: %ld\n", sent, recvd, timeouts, corrupt);
    printf("Packet Delivery Ratio (PDR): %.1f%%   Loss: %.1f%%\n", sent ? 100.0 * recvd / sent : 0, sent ? 100.0 * (sent - recvd) / sent : 0);
    if (recvd) printf("RTT avg/min/max: %.3f / %.3f / %.3f ms   Jitter(max-min): %.3f ms\n", rtt_sum / recvd * 1e3, rtt_min * 1e3, rtt_max * 1e3, (rtt_max - rtt_min) * 1e3);
    if (rtt_sum > 0) printf("Throughput: %.1f bytes/s  (bytes out=%ld, in=%ld)\n", bytes_in / rtt_sum, bytes_out, bytes_in);
    printf("======================\n");
}

int main(int argc, char **argv) {
    const char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv = {TIMEOUT_S, 0}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(PORT); inet_pton(AF_INET, ip, &a.sin_addr);
    char line[300];
    for (;;) {
        printf("\nEnter domain (stats/cache/quit): "); if (!fgets(line, sizeof line, stdin)) break;
        line[strcspn(line, "\r\n")] = 0; if (!*line) continue;
        if (!strcmp(line, "quit")) break;
        if (!strcmp(line, "stats")) { print_stats(); continue; }
        if (!strcmp(line, "cache")) { for (int i = 0; i < CACHE_N; i++) if (cache[i].used) printf("  %s -> %s (ttl left %lds)\n", cache[i].name, cache[i].ip, (long)(cache[i].exp - time(NULL))); continue; }
        queries++;
        for (char *p = line; *p; p++) *p = tolower((unsigned char)*p);
        Cache *c = cache_find(line);
        if (c) { hits++; printf("[CACHE HIT] %s -> %s\n", line, c->ip); continue; }

        char req[300], out[340], buf[512]; int done = 0;
        snprintf(req, sizeof req, "QUERY|%s", line); crc_pack(out, sizeof out, req);
        for (int at = 1; at <= MAX_RETRY && !done; at++) {
            double t0 = now();
            sendto(s, out, strlen(out), 0, (struct sockaddr *)&a, sizeof a); sent++; bytes_out += strlen(out);
            ssize_t n = recvfrom(s, buf, sizeof buf - 1, 0, NULL, NULL);
            if (n < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK) { timeouts++; printf("[TIMEOUT] attempt %d/%d\n", at, MAX_RETRY); continue; } perror("recvfrom"); break; }
            double rtt = now() - t0; buf[n] = 0;
            if (!crc_unpack(buf)) { corrupt++; printf("[CRC ERROR] corrupted reply, retrying\n"); continue; }
            recvd++; bytes_in += n; rtt_sum += rtt; if (rtt < rtt_min) rtt_min = rtt; if (rtt > rtt_max) rtt_max = rtt;
            char *save, *tok = strtok_r(buf, "|", &save);
            if (!strcmp(tok, "OK")) {
                char *rip = strtok_r(NULL, "|", &save), *ttl = strtok_r(NULL, "|", &save);
                printf("[RESOLVED] %s -> %s  (RTT %.3f ms, TTL %s)\n", line, rip, rtt * 1e3, ttl);
                cache_put(line, rip, atoi(ttl));
            } else if (!strcmp(tok, "NXDOMAIN")) printf("[NXDOMAIN] %s does not exist (RTT %.3f ms)\n", line, rtt * 1e3);
            else printf("[ERROR] server says: invalid query format\n");
            done = 1;
        }
        if (!done) printf("[FAILED] could not resolve %s after %d attempts\n", line, MAX_RETRY);
    }
    print_stats(); close(s); return 0;
}
