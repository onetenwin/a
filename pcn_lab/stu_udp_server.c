
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int s;
    char roll[20], line[200], result[300];
    struct sockaddr_in server = {0}, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_DGRAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&server, sizeof(server));
    printf("UDP server running on port 9000\n");

    while (1) {
        int n = recvfrom(s, roll, sizeof(roll) - 1, 0,
                         (struct sockaddr *)&client, &len);
        if (n <= 0) continue;
        roll[n] = '\0';

        strcpy(result, "Student Record Not Found");
        FILE *f = fopen("students.csv", "r");

        if (f) {
            fgets(line, sizeof(line), f);

            while (fgets(line, sizeof(line), f)) {
                char r[20], name[50], dept[50];
                int sem;
                float cgpa;

                if (sscanf(line, "%19[^,],%49[^,],%49[^,],%d,%f",
                           r, name, dept, &sem, &cgpa) == 5 &&
                    strcmp(r, roll) == 0) {
                    snprintf(result, sizeof(result),
                             "Roll: %s\nName: %s\nDepartment: %s\nSemester: %d\nCGPA: %.2f",
                             r, name, dept, sem, cgpa);
                    break;
                }
            }
            fclose(f);
        }

        sendto(s, result, strlen(result) + 1, 0,
               (struct sockaddr *)&client, len);
    }

    close(s);
    return 0;
}
