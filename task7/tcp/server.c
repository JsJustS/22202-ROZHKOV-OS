#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8888
#define sockaddr struct sockaddr_in
#define BUFSIZE 4096

int serveClient(int clt_sock);

void main() {
	int srv_sock;
	int clt_sock;
	int client_count = 0;
	int i;
	int err;
	sockaddr srv_sockaddr;
	sockaddr clt_sockaddr;

	// Init server
	srv_sock = socket(AF_INET, SOCK_STREAM, 0);
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

	err = listen(srv_sock, 5);
	if (err == -1) {
		perror("Could not switch server socket to passive mode");
		close(srv_sock);
		return;
	}

	printf("Waiting for conections on 127.0.0.1:%d\n", PORT);

	int len = 0;
	while (1) {
		int clt_sock = accept(srv_sock, (sockaddr *) &clt_sockaddr, &len);

		if (clt_sock == -1) {
			perror("Could not accept client");
			close(srv_sock);
			return;
		}

		int pid = fork();
		if (pid == -1) {
			perror("Could not fork");
			close(srv_sock);
			return;
		}

		// fork() == 0: child, serves client, does not need server fd
		// fork() != 0: parent, accepts new clients untill death
		if (pid == 0) {
			close(srv_sock);
			serveClient(clt_sock);
			return;
		}
	}
}

int serveClient(int clt_sock) {
	char data[BUFSIZE + 1];
	int status_code;
	while (1) {
		int ret;
		ret = read(clt_sock, data, BUFSIZE);
		if (ret == -1) {
			perror("Could not read data from client.");
			status_code = -1;
			break;
		}
		if (ret == 0) {
			printf("Client %d closed.\n", getpid());
			status_code = 0;
			break;
		}

		data[BUFSIZE] = '\0';
		printf("%d > %s\n", getpid(), data);

		if (write(clt_sock, data, BUFSIZE) < 1) {
			perror("Could not write data to client.");
			status_code = -1;
			break;
		}
	}
	return status_code;
}