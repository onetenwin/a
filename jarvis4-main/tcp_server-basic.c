#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(){
	int server_socket, client_socket;
	struct sockaddr_in client, server;
	char buffer[1024];
	
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8080);

	bind(server_socket, (struct sockaddr *)&server,sizeof(server));

	listen(server_socket, 0);
	printf("server listening...\n");
	fflush(stdout);

	socklen_t len = sizeof(client);
	client_socket = accept(server_socket, (struct sockaddr *)&client, &len);

	printf("client connected");

	recv(client_socket, buffer, sizeof(buffer), 0);

	printf("%s\n", buffer);

	strcpy(buffer, "Hello from server");

	send(client_socket, buffer, strlen(buffer) + 1, 0);

	close(client_socket);
	close(server_socket);
}