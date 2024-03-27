#define STD_OUT 1

void myexit(int status) {
	asm (
		"movl %0, %%ebx;"
		"movl $1, %%eax;"
		"int $0x80;"
		:
		: "a"(status)
	);
}

void mywrite(int fd, const char *buf, int count) {
	asm (
		"movl $4, %%eax;"
		"movl %0, %%ebx;"
		"movl %1, %%ecx;"
		"movl %2, %%edx;"
		"int $0x80;"
		:
		: "b"(fd), "c" (buf), "d" (count)
	);
}

void _start() {
	mywrite(STD_OUT, "hello world\n", 12);
	myexit(0);
}