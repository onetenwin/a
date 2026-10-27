
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *handle(void *arg) {
    int c = *(int *)arg;
    free(arg);

    char roll[20], line[200], result[300] = "Student Record Not Found";
    recv(c, roll, sizeof(roll) - 1, 0);
    roll[strcspn(roll, "\r\n")] = '\0';

    FILE *f = fopen("students.csv", "r");
    if (f) {
        fgets(line, sizeof(line), f);  // Skip header

        while (fgets(line, sizeof(line), f)) {
            char r[20], name[50], dept[50];
            int sem;
            float cgpa;

            if (sscanf(line, "%19[^,],%49[^,],%49[^,],%d,%f",
                       r, name, dept, &sem, &cgpa) == 5 &&
                strcmp(r, roll) == 0) {
                snprintf(result, sizeof(result),
                         "Roll: %s\nName: %s\nDepartment: %s\nSemester: %d\nCGPA: %.2f\n",
                         r, name, dept, sem, cgpa);
                break;
            }
        }
        fclose(f);
    }

    send(c, result, strlen(result), 0);
    close(c);
    return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server = {0};

    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&server, sizeof(server));
    listen(s, 5);
    printf("Server running on port 9000\n");

    while (1) {
        int *c = malloc(sizeof(int));
        *c = accept(s, NULL, NULL);

        if (*c < 0) {
            free(c);
            continue;
        }

        pthread_t t;
        pthread_create(&t, NULL, handle, c);
        pthread_detach(t);
    }

    close(s);
    return 0;
}
