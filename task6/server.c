#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#define sockaddr struct sockaddr_un
const int MAX_CLIENTS = 5;

int serveClient(int clt_sock);

void main() {
	char x[100];
	x[0] = '\0';
	printf("hello: %d\n", strcmp(x, "hmmmm"));
	int srv_sock;
	int client_count = 0;
	int clt_socks[MAX_CLIENTS];
	int clt_pids[MAX_CLIENTS];
	int i;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		clt_socks[i] = -1;
		clt_pids[i] = -1;
	}

	sockaddr srv_sockaddr;
	sockaddr clt_sockaddrs[MAX_CLIENTS];
	int len = 0;
	int val = 0;
	int err;

	char *dsock_file = "./socks/dsock_file";

	// Init server
	srv_sock = socket(AF_UNIX, SOCK_STREAM | O_NONBLOCK, 0);
	if (srv_sock == -1) {
		perror("Failed on creating server socket");
		return;
	}

	memset(&srv_sockaddr, 0, sizeof(sockaddr));
	srv_sockaddr.sun_family = AF_UNIX;
	strcpy(srv_sockaddr.sun_path, dsock_file);

	unlink(dsock_file);
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

	while (1) {
		for (i = 0; i < MAX_CLIENTS; ++i) {
			if (clt_socks[i] != -1) {
				int status;
				int res = waitpid(clt_pids[i], &status, WNOHANG);
				if (res == -1) {
					perror("fail");
					return;
				} else if (res > 0) {
					close(clt_socks[i]);
					clt_socks[i] = -1;
					clt_pids[i] = -1;
					client_count--;
					printf("Client left: %d/%d. Status: %d\n", client_count, MAX_CLIENTS, WEXITSTATUS(res));
				}
			}
		}

		if (client_count < MAX_CLIENTS) {
			sockaddr clt_sockaddr;
			memset(&clt_sockaddr, 0, sizeof(sockaddr));
			int clt_sock = accept4(srv_sock, (sockaddr*) &clt_sockaddr, &len, O_NONBLOCK);
			if (clt_sock == -1) {
				if (errno == EWOULDBLOCK) {
        				puts("...");
        				sleep(1);
					continue;
      				}
				perror("Could not accept client");
				close(srv_sock);
				for (i = 0; i < MAX_CLIENTS; ++i) {
					if (clt_socks[i]!=-1) {
						close(clt_socks[i]);
						int st;
						wait(&st);
					}
				}
				return;
			}
			
			client_count++;
			printf("Client connected: %d/%d.\n", client_count, MAX_CLIENTS);
			for (i = 0; i < MAX_CLIENTS; ++i) {
				if (clt_socks[i] == -1) break;
			}
			clt_socks[i] = clt_sock;
			clt_sockaddrs[i] = clt_sockaddr;
			int clt_pid = fork();			
			if (clt_pid == 0) {
				int exitcode = serveClient(clt_sock);
				close(clt_sock);
				exit(exitcode);
			} else if (clt_pid < 0) {
				perror("Could not fork for client");
				close(clt_sock);
				for (i = 0; i < MAX_CLIENTS; ++i) {
					if (clt_socks[i]!=-1) {
						close(clt_socks[i]);
						int st;
						wait(&st);
					}
				}
				return;
			} else {
				clt_pids[i] = clt_pid;
			}
		}
	}
}

int serveClient(int clt_sock) {
	char data[4096];
	int ret;
	while (1) {
		ret = read(clt_sock, data, sizeof(data));
		//printf("[%d]", ret);
		if (ret == -1) continue;
		if (ret == 0) {
			puts("damn");
		}
		data[ret] = '\0';
		write(clt_sock, data, ret);
		puts(data);
		if (strcmp(data, "exit")==0) {
			puts("lol!");
			return 0;
		}
	}
	return -1;
}