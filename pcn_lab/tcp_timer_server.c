
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(9000);
    a.sin_addr.s_addr = INADDR_ANY;

    bind(s, (void *)&a, sizeof(a));
    listen(s, 5);
    printf("Server running...\n");

    int c = accept(s, NULL, NULL);
    char msg[100];
    int n = read(c, msg, sizeof(msg));

    if (n > 0) {
        sleep(5);  // Delay response to demonstrate timeout
        write(c, "ACK", 3);
    }

    close(c);
    close(s);
    return 0;
}
