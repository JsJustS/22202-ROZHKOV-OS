#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>

void recurStack();
void cycleHeap();
void mappingTest();
void signalHandler(int signal);

void main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "choose between \"stack\" or \"heap\" or \"map\"\n");
		return;
	}
	printf("My pid is %d\n", getpid());
	sleep(10);
	if(strcmp(argv[1], "stack")==0) {
		printf("starting recursion...\n");
		recurStack();
		return;
	} else if (strcmp(argv[1],"heap")==0) {
		cycleHeap();
		return;
	} else if (strcmp(argv[1], "map")==0) {
		mappingTest();
		return;
	}
	fprintf(stderr, "wrong argument passed\n");
}

void recurStack() {
	char buf[4096];
	sleep(2);
	recurStack();
}

void cycleHeap() {
	unsigned int count = 0;
	int size = 512*1024*8; //8
	long long int* buf = 0;
	long long int* temp;
	puts("1");
	while (count < 8) {
		temp = buf;
		buf = malloc(size*sizeof(long long int));
		buf[0] = temp;
		sleep(2);
		count++;
	}

	int i;
	for (i = count; i >= 0; ++i) {
		if (temp == 0) break;
		temp = buf[0];
		free(buf);
		sleep(2);
		buf = temp;
	}
}

void mappingTest() {
	signal(SIGSEGV, signalHandler);
	
	int pageSize = getpagesize();
	int* mappedMemory = mmap(NULL, pageSize * 10, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mappedMemory == (void *)-1) {
		fprintf(stderr, "Could not map memory: %s\n", strerror(errno));
		exit(2);
	} else {
		printf("I mapped on %p\n", mappedMemory);
	}
    	
	sleep(15);

	mprotect(mappedMemory, pageSize * 10, PROT_READ);
	printf("I protected memory from writing or executing on %p\n", mappedMemory);

	//generate segfault
	//mappedMemory[0] = 42;

	sleep(15);

	munmap(mappedMemory + pageSize * 4, pageSize * 2);
	printf("I unmapped memory from page %p to page %p\n", mappedMemory + pageSize * 4, mappedMemory + pageSize * 6);
	sleep(1000);
}

void signalHandler(int signal) {
	printf("hello yes i handled signal\n");
	//sleep(2);
	exit(2);
}