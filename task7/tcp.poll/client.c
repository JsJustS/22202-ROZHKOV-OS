#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#define sockaddr struct sockaddr_in

void main(int argc, char** argv) {
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

	char data[4096];
	while (1) {
		int ret;
		fgets(data, 4096, stdin);
		data[strlen(data)-1] = '\0';
		write(clt_sock, data, strlen(data));
		ret = read(clt_sock, data, sizeof(data));
		if (ret == -1) {
			printf("Failed reading...\n");
		} else {
			printf("Message FROM server: \"%s\"\n", data);
		}
		if (strcmp(data, "exit")==0) {
			close(clt_sock);
			exit(0);
		}
	}
}