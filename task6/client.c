#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#define sockaddr struct sockaddr_un

void main() {
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

	unlink(dsock_file_clt);
	int err = bind(clt_sock, (sockaddr*) &clt_sockaddr, sizeof(clt_sockaddr));
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