#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);                     


    server_addr.sin_family = AF_INET;                                           
    server_addr.sin_addr.s_addr = INADDR_ANY;   
// ethernet or wifi or localhost
    server_addr.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
// bind() assigns an IP address and port number to a socket.
    listen(server_fd, 5);
//listen() puts a TCP server socket into passive mode, meaning it is ready to accept incoming client connections.
    printf("Server waiting...\n");

    client_fd = accept(server_fd, NULL, NULL); 
//(listening socket, stores client ip addr, size of client IP address)

    read(client_fd, buffer, sizeof(buffer));
    printf("Client says: %s\n", buffer);

    close(client_fd);
    close(server_fd);

    return 0;
}