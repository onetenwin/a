/* Assignment 3 (Advanced) - HTTPS client using OpenSSL.
   Build: gcc a3_https_client.c -o a3_https_client -lssl -lcrypto
   Usage: ./a3_https_client example.com [/path] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "example.com", *path = argc > 2 ? argv[2] : "/";
    struct addrinfo h = {0}, *res; h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, "443", &h, &res)) { fprintf(stderr, "DNS failed\n"); return 1; }
    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    double t0 = now();
    if (connect(s, res->ai_addr, res->ai_addrlen) < 0) { perror("connect"); return 1; }
    double t_tcp = now();

    SSL_library_init(); SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_default_verify_paths(ctx);          /* trusted CA store */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL); /* verify certificate chain */
    SSL *ssl = SSL_new(ctx); SSL_set_fd(ssl, s);
    SSL_set_tlsext_host_name(ssl, host);            /* SNI */
    SSL_set1_host(ssl, host);                       /* hostname check */
    if (SSL_connect(ssl) <= 0) { ERR_print_errors_fp(stderr); return 1; }
    double t_hs = now();

    long vr = SSL_get_verify_result(ssl);
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    X509 *cert = SSL_get1_peer_certificate(ssl);
#else
    X509 *cert = SSL_get_peer_certificate(ssl);
#endif
    printf("TLS version     : %s\nCipher suite    : %s\n", SSL_get_version(ssl), SSL_get_cipher_name(ssl));
    printf("Cert verify     : %s (code %ld)\n", vr == X509_V_OK ? "OK" : X509_verify_cert_error_string(vr), vr);
    if (cert) {
        char *sub = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0), *iss = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Cert subject    : %s\nCert issuer     : %s\n", sub, iss); OPENSSL_free(sub); OPENSSL_free(iss); X509_free(cert);
    }
    char req[1024]; snprintf(req, sizeof req, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    double t_req = now();
    SSL_write(ssl, req, strlen(req));
    char buf[8192]; int n; size_t total = 0; double t_first = 0; int shown = 0;
    while ((n = SSL_read(ssl, buf, sizeof buf)) > 0) {
        if (!t_first) t_first = now();
        if (!shown) { printf("\n--- first bytes of response ---\n%.400s\n-------------------------------\n", buf); shown = 1; }
        total += n;
    }
    double t_end = now();
    SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(ctx); close(s); freeaddrinfo(res);
    if (!t_first) t_first = t_end;
    double dl = t_end - t_first;
    printf("TCP conn time   : %.3f ms\nSSL handshake   : %.3f ms\n", (t_tcp - t0) * 1e3, (t_hs - t_tcp) * 1e3);
    printf("RTT (req->1st)  : %.3f ms\nData received   : %zu bytes\n", (t_first - t_req) * 1e3, total);
    printf("Transfer rate   : %.3f KB/s\nTotal time      : %.3f ms\n", dl > 0 ? total / dl / 1024.0 : 0.0, (t_end - t0) * 1e3);
    return 0;
}
