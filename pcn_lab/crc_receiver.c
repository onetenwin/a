
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define G "10011"

void divide(char *bits) {
    int n = strlen(bits), g = strlen(G);

    for (int i = 0; i <= n - g; i++) {
        if (bits[i] == '1')
            for (int j = 0; j < g; j++)
                bits[i+j] = (bits[i+j] == G[j]) ? '0' : '1';
    }
}

int main() {
    int s, c, n;
    char frame[1024], check[1024];
    struct sockaddr_in a = {0};

    s = socket(AF_INET, SOCK_STREAM, 0);
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (void *)&a, sizeof(a));
    listen(s, 5);
    printf("CRC receiver waiting...\n");

    c = accept(s, NULL, NULL);
    n = 0;
    while (n < sizeof(frame)-1) {
        int r = recv(c, frame+n, sizeof(frame)-1-n, 0);
        if (r <= 0) break;
        n += r;
    }
    frame[n] = '\0';

    printf("Received frame: %s\n", frame);

    strcpy(check, frame);
    divide(check);

    if (strspn(check, "0") == strlen(check))
        printf("Result: No error detected\n");
    else
        printf("Result: Error detected\n");

    close(c);
    close(s);
    return 0;
}
