#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>

#define MAX_DEPTH 10

void recursion(int depth);
int childProcEntryFunc();

int main(int argc, char **argv) {
	const int STACK_SIZE = 1024;

	void* stack_memory = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
	if (!stack_memory) {
		perror("Could not mmap() stack for child");
		exit(1);
	}

	if (clone(childProcEntryFunc, stack_memory + STACK_SIZE, SIGCHLD, NULL) == -1) {
		perror("Could not clone child");
		exit(1);
	}

	// Ждём ребёнка
	int status;
	if (wait(&status) == -1) {
		perror("wait() error");
		exit(1);
	}

	// Дождались, записываем дамп стека в файл
	char filename[] = "stack.bin";
	int file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (file < 0) {
		perror("Could not open file");
		exit(1);
	}

	if (write(file, stack_memory, STACK_SIZE) == -1) {
		perror("Could not write to file");
	}

	close(file);
	return 0;
}

int childProcEntryFunc() {
	recursion(0);
	return 0;
}

void recursion(int depth) {
	if (depth >= MAX_DEPTH) {
		return;
	}
	// Легко будет увидеть значение в hexdump
	char charDepth = (char) depth + '0';
	char hello_world[] = "hello world";
	recursion(depth + 1);
}