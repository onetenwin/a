/* Assignment 2(b) - UDP server. Build: gcc a2_udp_server.c -o a2_udp_server */
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "student_db.h"
#include "crc32.h"
#define PORT 9090

int main(void) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in a = {0}, c; socklen_t cl = sizeof c;
    a.sin_family = AF_INET; a.sin_addr.s_addr = INADDR_ANY; a.sin_port = htons(PORT);
    if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) { perror("bind"); return 1; }
    printf("UDP server on port %d\n", PORT);
    for (;;) {
        char buf[256], payload[256], out[300]; Student st;
        ssize_t n = recvfrom(s, buf, sizeof buf - 1, 0, (struct sockaddr *)&c, &cl);
        if (n <= 0) continue;
        buf[n] = 0;
        if (!crc_unpack(buf)) { printf("corrupted datagram dropped\n"); continue; }
        printf("query from %s:%d -> %s\n", inet_ntoa(c.sin_addr), ntohs(c.sin_port), buf);
        int r = find_student(buf, &st);
        if (r == 1) snprintf(payload, sizeof payload, "Roll Number: %s, Name: %s", st.roll, st.name);
        else snprintf(payload, sizeof payload, "Student Record Not Found");
        crc_pack(out, sizeof out, payload);
        sendto(s, out, strlen(out), 0, (struct sockaddr *)&c, cl);
    }
}
