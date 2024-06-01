#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define sockaddr struct sockaddr_in
#define PORT 8888
void main() {
	int clt_sock;
	sockaddr srv_sockaddr;

	clt_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (clt_sock == -1) {
		perror("Could not open socket");
		exit(1);
	}

	memset(&srv_sockaddr, 0, sizeof(sockaddr));

	srv_sockaddr.sin_family = AF_INET;
	srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	srv_sockaddr.sin_port = htons(PORT);

	char data[4096];
	while (1) {
		int ret, len;
		fgets(data, 4096, stdin);
		data[strlen(data)-1] = '\0';

		if (-1 == sendto(clt_sock, data, sizeof(data), 0, (sockaddr*) &srv_sockaddr, sizeof(srv_sockaddr))) {
			perror("Could not send data to server");
			close(clt_sock);
			exit(0);
		}

		ret = recvfrom(clt_sock, data, sizeof(data), 0, (sockaddr*) &srv_sockaddr, &len);
		if (ret == -1) {
			printf("Failed reading...\n");
		} else {
			printf("Message FROM server: \"%s\"\n", data);
		}
		
		if (strcmp(data, "exit")==0 || strcmp(data, "close")==0) {
			close(clt_sock);
			exit(0);
		}
	}
}