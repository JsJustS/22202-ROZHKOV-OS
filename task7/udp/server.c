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
#define BUFSIZE 4096

void serveClient(int srv_sock);

void main() {
	int srv_sock;
	int clt_sock;
	int i;
	int err;
	sockaddr srv_sockaddr;
	sockaddr clt_sockaddr;

	// Init server
	srv_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (srv_sock == -1) {
		perror("Failed on creating server socket");
		return;
	}

	int option = 1;
	err = setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if (err == -1) {
		perror("Could not set socket option");
		close(srv_sock);
		return;
	}

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

	while (1) {
		serveClient(srv_sock);
	}
}

void serveClient(int srv_sock) {
	char data[BUFSIZE+1];
	int ret, len = sizeof(sockaddr);
	sockaddr clt_sockaddr;

	ret = recvfrom(srv_sock, data, sizeof(data), 0, (sockaddr*) &clt_sockaddr, &len);
	if (ret == -1) {
		perror("Could not read data from client.");
		exit(1);
	}
	if (ret == 0) {
		printf("Client closed.\n");
		return;
	}
	data[ret] = '\0';
	printf("> %s\n", data);

	if (sendto(srv_sock, data, sizeof(data), 0, (sockaddr*) &clt_sockaddr, len) < 0) {
		perror("Could not send data to client");
		return;
	}
	return;
}