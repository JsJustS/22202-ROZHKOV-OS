#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define sockaddr struct sockaddr_in
#define PORT 8888
const int MAX_CLIENTS = 5;

int serveClient(int srv_sock);

void main() {
	int srv_sock;
	int i;

	sockaddr srv_sockaddr;
	int len = 0;
	int val = 0;
	int err;

	// Init server
	srv_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (srv_sock == -1) {
		perror("Failed on creating server socket");
		return;
	}
	int option = 1;
	setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	memset(&srv_sockaddr, 0, sizeof(sockaddr));
	srv_sockaddr.sin_family = AF_INET;
    	srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	srv_sockaddr.sin_port = htons(PORT);

	err = bind(srv_sock, (sockaddr*) &srv_sockaddr, sizeof(srv_sockaddr));
	if (err == -1) {
		perror("Could not bind server socket");
		close(srv_sock);
		return;
	}

	printf("Waiting for UDP clients on port %d\n", PORT);

	int running = 1;
	while (running) {
		running = serveClient(srv_sock);
	}
}

int serveClient(int srv_sock) {
	char data[4096];
	int ret, len = sizeof(sockaddr);
	sockaddr clt_sockaddr;

	ret = recvfrom(srv_sock, data, sizeof(data), 0, (sockaddr*) &clt_sockaddr, &len);
	if (ret < 1) return 1; // error on receive, try again
	data[ret] = '\0';

	if (-1 == sendto(srv_sock, data, sizeof(data), 0, (sockaddr*) &clt_sockaddr, len)) {
		perror("Could not send data to client");
		return 0;
	}
	puts(data);

	if (strcmp(data, "close")==0) return 0;
	if (strcmp(data, "exit")==0) {
		puts("Client left");
	}
	return 1;
}