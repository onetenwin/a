#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(){
	int client_socket;
	struct sockaddr_in client,server;
	char buffer[1024];

	client_socket = socket(AF_INET, SOCK_DGRAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(8080);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	socklen_t len = sizeof(server);
	
	strcpy(buffer,"Hello from client");

	sendto(client_socket, buffer, strlen(buffer) + 1, 0 , (struct sockaddr *)&server, len);

	recvfrom(client_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &len);

	printf("%s\n",buffer);

	close(client_socket);
}