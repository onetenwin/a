#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char *msg = "Hello Server";

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); 


    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
// inet_pton() converts an IP address from text format to binary network format.

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
// connect() is used by a client socket to establish a connection with a server.
    write(sock, msg, strlen(msg));
// sends the message stored in msg through the connected TCP socket.
    close(sock);

    return 0;
}