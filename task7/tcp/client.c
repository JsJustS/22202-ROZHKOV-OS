#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define sockaddr struct sockaddr_in
#define BUFSIZE 4096

void input(char *buf){
	printf("> ");
	fflush(stdout);
	int n = read(STDIN_FILENO, buf, BUFSIZE);
	if (n == -1){
		perror("Could not read input data.");
		exit(1);
	}
	buf[n-1] = '\0';
}

void main(int argc, char** argv) {
	signal(SIGPIPE, SIG_IGN);
	int PORT = 8888;
	if (argc > 1) {
		PORT = atoi(argv[1]);
	}
	int clt_sock;
	sockaddr clt_sockaddr, srv_sockaddr;

	clt_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (clt_sock == -1) {
		perror("Could not open socket");
		exit(1);
	}

	memset(&srv_sockaddr, 0, sizeof(sockaddr));

	srv_sockaddr.sin_family = AF_INET;
	srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_sockaddr.sin_port = htons(PORT);

	int err = connect(clt_sock, (sockaddr*) &srv_sockaddr, sizeof(srv_sockaddr));
	if (err == -1) {
		perror("Connection failed");
		close(clt_sock);
		exit(1);
	}

	char data[BUFSIZE + 1];
	int status_code;
	while (1) {
		input(data);
		if (strncmp(data, "exit", 4)==0) {
			status_code = 0;
			break;
		}

		if (write(clt_sock, data, BUFSIZE) < 1) {
			perror("Could not write to the socket from client");
			status_code = 1;
			break;
		}

		int ret = read(clt_sock, data, sizeof(data));
		if (ret == -1) {
			perror("Could not read from the server socket");
			status_code = 1;
			break;
		}
		printf("< %s\n", data);
	}
}