#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define sockaddr struct sockaddr_un
#define BUFSIZE 4096
#define MAX_CLIENTS 5

int serveClient(int clt_sock);

void cleanUp(int srv_sock, int g);

void main() {
	int srv_sock;
	int clt_sock;
	int client_count = 0;
	int i;
	int err;
	sockaddr srv_sockaddr;
	sockaddr clt_sockaddr;

	char *dsock_file = "./socks/dsock_file";

	// Init server
	srv_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (srv_sock == -1) {
		perror("Failed on creating server socket");
		return;
	}

	memset(&srv_sockaddr, 0, sizeof(sockaddr));
	srv_sockaddr.sun_family = AF_UNIX;
	strcpy(srv_sockaddr.sun_path, dsock_file);

	err = unlink(dsock_file);
	if (err == -1) {
		perror("Could not unlink socket");
		return;		
	}

	err = bind(srv_sock, (sockaddr*) &srv_sockaddr, sizeof(srv_sockaddr));
	if (err == -1) {
		perror("Could not bind server socket");
		close(srv_sock);
		return;
	}

	err = listen(srv_sock, MAX_CLIENTS);
	if (err == -1) {
		perror("Could not switch server socket to passive mode");
		close(srv_sock);
		return;
	}

	printf("Waiting for conections on \"%s\"\n", dsock_file);

	int len = 0;
	while (1) {
		int clt_sock = accept(srv_sock, (sockaddr *) &clt_sockaddr, &len);

		if (clt_sock == -1) {
			perror("Could not accept client");
			close(srv_sock);
			return;
		}
		client_count++;

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
	char data[BUFSIZE+1];
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
			printf("Client %d closed.\n", gettid());
			status_code = 0;
			break;
		}

		data[BUFSIZE] = '\0';
		printf("%d > %s\n", gettid(), data);
		if (strcmp(data, "close")==0) {
			status_code = 1;
			break;
		}

		if (write(clt_sock, data, BUFSIZE) < 1) {
			perror("Could not write data to client.");
			status_code = -1;
			break;
		}
	}
	return status_code;
}