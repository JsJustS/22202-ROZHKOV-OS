#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>

typedef struct user_regs_struct Regs;

void printSysCall(long id, pid_t proc, Regs* regs);
void printSysCallVars(long id);
void peekData(char* ptr, int len, pid_t proc, long start);

int main(int argc, char* argv[]) {

	if (argc < 2) {
		printf("Usage: %s <./path/to/executable> [args...]\n", argv[0]);
		return 1;
	}

	pid_t child;
	int status;
	long orig_eax;

	child = fork();

	if (child == 0) {
		// Дочерний процесс
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execv(argv[1], &argv[1]);
	} else {
		// Родительский процесс
		waitpid(child, &status, 0);
		//Regs* regs = malloc(sizeof(Regs));
		Regs regs;

		while (WIFSTOPPED(status)) {
			orig_eax = ptrace(PTRACE_PEEKUSER, child, 4 * ORIG_EAX, NULL);
			ptrace(PTRACE_GETREGS, child, 0, &regs);
			printSysCall(orig_eax, child, &regs);
			ptrace(PTRACE_SYSCALL, child, NULL, NULL);
			waitpid(child, &status, 0);
		}
		//free(regs);
	}
	return 0;
}

void printSysCall(long id, pid_t proc, Regs* regs) {
	switch (id) {
		case 0:
			puts("restart syscall");
			break;
		case 1:
			printf("exit(%ld) = ?\n", regs->ebx);
			break;
		case 2:
			puts("fork");
			break;
		case 3:
			puts("read");
			break;
		case 4: {
			char message[1000];
			peekData(message, regs->edx, proc, regs->ecx);
			printf("write(%ld, \"%s\", %ld) = %d\n", regs->ebx, message, regs->edx, regs->eax);
			break;
		}
		case 5:
			puts("open");
			break;
		case 6:
			puts("close");
			break;
		case 7:
			puts("waitpid");
			break;
		case 8:
			puts("creat");
			break;
		case 9:
			puts("link");
			break;
		case 10:
			puts("unlink");
			break;
		case 11: 
			printf("execve = %ld\n", regs->eax);
			break;
		default:
			printf("syscall with id: %ld\n", id);
	}
}

void peekData(char* ptr, int len, pid_t proc, long start) {
	char* temp_char = ptr;
	int j = 0;
	long temp_long;
	while(j < len) {
		temp_long = ptrace(PTRACE_PEEKDATA, proc, start + (j*sizeof(long)) , NULL);
		memcpy(temp_char, &temp_long, sizeof(long));		
		temp_char += sizeof(long);
		++j;
		
	}
	ptr[len] = '\0';
	screen(ptr, len);
}

void screen(char* ptr, int len) {
	char* temp = malloc(sizeof(char)*len*2);
	if (temp == NULL) return;

	int i = 0, j = 0;
	for (; i < len; ++i, ++j) {
		if (ptr[i] == '\n') {
			temp[j++] = '\\';
			temp[j] = 'n';
		} else if (ptr[i] == '\t') {
			temp[j++] = '\\';
			temp[j] = 't';
		} else {
			temp[j] = ptr[i];
		}
	}
	temp[++j] = '\0';
	strcpy(ptr, temp);
	free(temp);
}