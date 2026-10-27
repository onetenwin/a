/* Assignment 3 - HTTP client over raw TCP sockets.
   Build: gcc a3_http_client.c -o a3_http_client
   Usage: ./a3_http_client http://localhost:8000/sample.pdf   (or run with no args and type URL) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

int main(int argc, char **argv) {
    char url[1024];
    if (argc > 1) snprintf(url, sizeof url, "%s", argv[1]);
    else { printf("Enter URL: "); if (!fgets(url, sizeof url, stdin)) return 1; url[strcspn(url, "\r\n")] = 0; }

    /* ---- parse URL ---- */
    char host[256] = "", port[8] = "80", path[768] = "/";
    const char *p = url;
    if (!strncmp(p, "http://", 7)) p += 7;
    else if (!strncmp(p, "https://", 8)) { fprintf(stderr, "Use a3_https_client for https\n"); return 1; }
    size_t i = 0; while (*p && *p != ':' && *p != '/' && i < sizeof host - 1) host[i++] = *p++; host[i] = 0;
    if (*p == ':') { p++; i = 0; while (*p && *p != '/' && i < sizeof port - 1) port[i++] = *p++; port[i] = 0; }
    if (*p == '/') snprintf(path, sizeof path, "%s", p);
    printf("Host: %s\nPort: %s\nPath: %s\n", host, port, path);

    /* ---- DNS + connect ---- */
    double t_start = now();
    struct addrinfo hints = {0}, *res, *rp; hints.ai_socktype = SOCK_STREAM; hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host, port, &hints, &res)) { fprintf(stderr, "DNS failure\n"); return 1; }
    double t_dns = now();
    int s = -1;
    for (rp = res; rp; rp = rp->ai_next) {
        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s < 0) continue;
        if (connect(s, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(s); s = -1;
    }
    freeaddrinfo(res);
    if (s < 0) { perror("connect"); return 1; }
    double t_conn = now();

    /* ---- send GET ---- */
    char req[1400], hosthdr[300];
    if (strcmp(port, "80")) snprintf(hosthdr, sizeof hosthdr, "%s:%s", host, port); else snprintf(hosthdr, sizeof hosthdr, "%s", host);
    snprintf(req, sizeof req, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: CN-Lab-Client/1.0\r\nConnection: close\r\n\r\n", path, hosthdr);
    double t_send = now();
    send(s, req, strlen(req), 0);

    /* ---- receive ---- */
    static char hdr[16384]; size_t hl = 0; int hdr_done = 0, status = 0;
    char buf[8192]; ssize_t n; size_t body = 0; FILE *f = NULL; double t_first = 0; char fname[300] = "";
    while ((n = recv(s, buf, sizeof buf, 0)) > 0) {
        if (!t_first) t_first = now();
        if (!hdr_done) {
            if (hl + n >= sizeof hdr) { fprintf(stderr, "header too large\n"); break; }
            memcpy(hdr + hl, buf, n); hl += n; hdr[hl] = 0;
            char *e = strstr(hdr, "\r\n\r\n");
            if (e) {
                hdr_done = 1; *e = 0; sscanf(hdr, "HTTP/%*s %d", &status);
                printf("\n--- Response headers ---\n%s\n------------------------\n", hdr);
                if (status == 200) {
                    const char *b = strrchr(path, '/'); b = (b && b[1]) ? b + 1 : "index.html";
                    snprintf(fname, sizeof fname, "downloaded_%s", b); f = fopen(fname, "wb");
                }
                size_t off = (e + 4) - hdr, rem = hl - off;
                if (f && rem) fwrite(hdr + off, 1, rem, f);
                body += rem;
            }
        } else { if (f) fwrite(buf, 1, n, f); body += n; }
    }
    double t_end = now();
    close(s); if (f) fclose(f);
    if (!t_first) t_first = t_end;

    /* ---- metrics ---- */
    double resp = t_first - t_send, dl = t_end - t_first, total = t_end - t_start;
    printf("\nStatus code     : %d\n", status);
    if (f) printf("Saved to        : %s\n", fname);
    printf("DNS time        : %.3f ms\nTCP connect     : %.3f ms\n", (t_dns - t_start) * 1e3, (t_conn - t_dns) * 1e3);
    printf("Response time   : %.3f ms  (request sent -> first byte)\n", resp * 1e3);
    printf("Download time   : %.3f ms  (first byte -> close)\n", dl * 1e3);
    printf("Total time      : %.3f ms\n", total * 1e3);
    printf("Data size       : %zu bytes (body), %zu bytes (with headers)\n", body, body + hl - (hdr_done ? 0 : 0));
    printf("Throughput      : %.3f KB/s (%.3f Mbps)\n", dl > 0 ? body / dl / 1024.0 : 0.0, dl > 0 ? body * 8.0 / dl / 1e6 : 0.0);
    return status == 200 ? 0 : 2;
}
