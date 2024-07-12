#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>

#define sockaddr struct sockaddr_un
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
	printf("[%d]\n", n);
}

void main() {
	signal(SIGPIPE, SIG_IGN);
	int err;
	int clt_sock;
	sockaddr clt_sockaddr, srv_sockaddr;

	char *dsock_file = "./socks/dsock_file";
	char strpid[100];
	sprintf(strpid, "%d\0", getpid());
	char dsock_file_clt[512];
	snprintf(dsock_file_clt, sizeof(dsock_file_clt), "%s_%s", dsock_file, strpid);

	clt_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (clt_sock == -1) {
		perror("Could not open socket");
		exit(1);
	}

	memset(&clt_sockaddr, 0, sizeof(sockaddr));
	clt_sockaddr.sun_family = AF_UNIX;
	strcpy(clt_sockaddr.sun_path, dsock_file_clt);

	FILE* ptr;
	ptr = fopen(dsock_file_clt, "w");
	if (ptr == 0) {
		perror("Could not create socket file");
		exit(1);
	}
	fclose(ptr);

	err = unlink(dsock_file_clt);
	if (err == -1) {
		perror("Could not unlink socket");
		exit(1);		
	}

	err = bind(clt_sock, (sockaddr*) &clt_sockaddr, sizeof(clt_sockaddr));
	if (err == -1) {
		perror("Bind fail");
		close(clt_sock);
		exit(1);
	}

	memset(&srv_sockaddr, 0, sizeof(sockaddr));
	srv_sockaddr.sun_family = AF_UNIX;
	strcpy(srv_sockaddr.sun_path, dsock_file);

	err = connect(clt_sock, (sockaddr*) &srv_sockaddr, sizeof(srv_sockaddr));
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

	close(clt_sock);
	exit(status_code);
}