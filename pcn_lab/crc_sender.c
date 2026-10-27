
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define G "10011"

void divide(char *bits) {
    int n = strlen(bits), g = strlen(G);

    for (int i = 0; i <= n-g; i++) {
        if (bits[i] == '1')
            for (int j = 0; j < g; j++)
                bits[i+j] = (bits[i+j] == G[j]) ? '0' : '1';
    }
}

int main() {
    int s;
    char data[50], bits[500], temp[510], frame[520];
    struct sockaddr_in a = {0};

    printf("Enter data: ");
    scanf("%49s", data);

    bits[0] = '\0';
    for (int i = 0; data[i]; i++) {
        char b[9];
        for (int j = 7; j >= 0; j--)
            b[7-j] = ((data[i] >> j) & 1) + '0';
        b[8] = '\0';
        strcat(bits, b);
    }

    printf("Binary data: %s\n", bits);

    strcpy(temp, bits);
    strcat(temp, "0000");
    divide(temp);

    int len = strlen(bits);
    strcpy(frame, bits);
    strncat(frame, temp + len, 4);
    printf("CRC frame: %s\n", frame);

    char choice;
    printf("Introduce bit error? (y/n): ");
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y')
        frame[0] = (frame[0] == '0') ? '1' : '0';

    printf("Sending frame: %s\n", frame);

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);

    if (connect(s, (void *)&a, sizeof(a)) < 0) {
        perror("Connect");
        close(s);
        return 1;
    }

    send(s, frame, strlen(frame), 0);
    shutdown(s, SHUT_WR);
    close(s);
    return 0;
}
