#define SYS_write 4

void mywrite(int fd, const char *buf, int count) {
	syscall(SYS_write, fd, buf, count);
}

void main() {
	mywrite(1, "Hello world\n", 12);
}