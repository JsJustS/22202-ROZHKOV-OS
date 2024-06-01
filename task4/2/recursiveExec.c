void main(int argc, char** argv) {
	printf("My pid is %d\n", getpid());
	sleep(1);
	execve(argv[0], argv);
	printf("Hello world from %d!!!!\n", getpid());
}