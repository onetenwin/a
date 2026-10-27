/* Assignment 2(a) - TCP multi-client server (pthread). Build: gcc a2_tcp_server.c -o a2_tcp_server -pthread */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "student_db.h"
#define PORT 8080

void *handle_client(void *arg) {
    int fd = *(int *)arg; free(arg);
    char roll[64], reply[512]; Student s;
    ssize_t n = recv(fd, roll, sizeof roll - 1, 0);
    if (n > 0) {
        roll[n] = 0; trim(roll);
        printf("[thread %lu] query: %s\n", (unsigned long)pthread_self(), roll);
        int r = find_student(roll, &s);
        if (r == 1)
            snprintf(reply, sizeof reply, "Roll Number: %s\nName: %s\nDepartment: %s\nSemester: %s\nCGPA: %s\n",
                     s.roll, s.name, s.dept, s.sem, s.cgpa);
        else snprintf(reply, sizeof reply, "Student Record Not Found\n");
        send(fd, reply, strlen(reply), 0);
    }
    close(fd);                       /* graceful close */
    return NULL;
}

int main(void) {
    int srv = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_addr.s_addr = INADDR_ANY; a.sin_port = htons(PORT);
    if (bind(srv, (struct sockaddr *)&a, sizeof a) < 0) { perror("bind"); return 1; }
    listen(srv, 10);
    printf("TCP server listening on port %d\n", PORT);
    for (;;) {
        struct sockaddr_in c; socklen_t cl = sizeof c;
        int *fd = malloc(sizeof(int));
        *fd = accept(srv, (struct sockaddr *)&c, &cl);
        if (*fd < 0) { perror("accept"); free(fd); continue; }
        printf("client %s:%d connected\n", inet_ntoa(c.sin_addr), ntohs(c.sin_port));
        pthread_t t; pthread_create(&t, NULL, handle_client, fd); pthread_detach(t);
    }
}
