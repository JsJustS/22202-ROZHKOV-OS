#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define PORT 8888
#define sockaddr struct sockaddr_in
const int MAX_CLIENTS = 5;

void closeClient(int fd, int* socks);
int serveClient(int clt_sock);

void main() {
	int srv_sock;
	int clt_socks[MAX_CLIENTS];
	int i;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		clt_socks[i] = -1;
	}

	sockaddr srv_sockaddr;
	sockaddr clt_sockaddr;
	int len = 0;
	int val = 0;
	int err;

	// Init server
	srv_sock = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
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

	err = listen(srv_sock, MAX_CLIENTS);
	if (err == -1) {
		perror("Could not switch server socket to passive mode");
		close(srv_sock);
		return;
	}

	printf("Waiting for conections on 127.0.0.1:%d\n", PORT);

	int running = 1;
	while (running) {
		struct pollfd fds[MAX_CLIENTS+1];
		int ret;
		int fds_count = 0;
		int timeout = 1000;

		fds[0].fd = srv_sock;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds_count++;
		// rebuild fds list
		for (i = 0; i < MAX_CLIENTS; ++i) {
			if (clt_socks[i] != -1) {
				fds[fds_count].fd = clt_socks[i];
				fds[fds_count].events = POLLIN;
				fds[fds_count].revents = 0;
				fds_count++;
			}
		}

		ret = poll(fds, fds_count, timeout);
		if (ret == 0) {
			puts("..."); continue;
		} else if (ret > 0) {
			if (fds[0].revents == POLLIN) {
				puts("New connection avaliable");
				memset(&clt_sockaddr, 0, sizeof(sockaddr));
				for (i = 0; i < MAX_CLIENTS; ++i) if (clt_socks[i] == -1) break;
				clt_socks[i] = accept(srv_sock, (sockaddr*)&clt_sockaddr, &len);
				if (clt_socks[i] == -1) {
					perror("accept() failed");
					close(srv_sock);
					exit(1);
				}
			}
		}

		int j;
		for (j = 1; j < fds_count; ++j) {
			if (fds[j].revents == POLLIN) {
				ret = serveClient(fds[j].fd);
				if (ret == 1) {
					printf("Client closed: %d/%d.\n", fds_count-2, MAX_CLIENTS);
					closeClient(fds[j].fd, clt_socks);
				} else if (ret == 2) {
					puts("Closing server");
					running = 0;
				}
			}
		}
	}

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (clt_socks[i] != -1) {
			close(clt_socks[i]);
		}
	}
	close(srv_sock);
}

int serveClient(int clt_sock) {
	char data[4096];
	int ret;
	ret = read(clt_sock, data, sizeof(data));
	if (ret < 1) return 1;
	data[ret] = '\0';
	if (write(clt_sock, data, ret) < 1) {
		perror("Could not write to client");
		return 1;
	}
	puts(data);
	if (strcmp(data, "exit")==0) {
		return 1;
	} else if (strcmp(data, "close")==0) {
		return 2;
	}
	return 0;
}

void closeClient(int fd, int* socks) {
	int i;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (fd == socks[i]) {
			break;
		}
	}

	socks[i] = -1;
	close(fd);
}