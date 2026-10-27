#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(){
	int client_socket;
	struct sockaddr_in client,server;
	char buffer[1024];

	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(8080);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	connect(client_socket, (struct sockaddr *)&server, sizeof(server));

	strcpy(buffer,"Hello from client");

	send(client_socket, buffer, strlen(buffer) + 1, 0);
	recv(client_socket, buffer,sizeof(buffer), 0);

	printf("%s",buffer);

	close(client_socket);
}