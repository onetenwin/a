#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(){
	int server_socket;
	struct sockaddr_in client, server;
	char buffer[1024];

	server_socket = socket(AF_INET,SOCK_DGRAM, 0);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8080);

	bind(server_socket, (struct sockaddr *)&server, sizeof(server));

	printf("server is waiting\n");

	socklen_t len = sizeof(client);
	recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client, &len);

	printf("%s\n",buffer);

	strcpy(buffer,"Hello from server");

	sendto(server_socket, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&client, len);
	
	close(server_socket);
}