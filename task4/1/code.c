void main();
char* generatePointer();
void generateBuffer();
void changeEnvVar();

char global_init = 1;
char global_uninit;
const char global_const;

void main() {
	char func_local;
	static char func_static;
	const char func_const;

	printf("Addresses:\n");
	printf("global_init\t\t%p\n", &global_init);
	printf("global_uninit\t\t%p\n", &global_uninit);
	printf("global_const\t\t%p\n", &global_const);
	printf("func_local\t\t%p\n", &func_local);
	printf("func_static\t\t%p\n", &func_static);
	printf("func_const\t\t%p\n\n", &func_const);

	printf("from popped stack\t%p\n\n", generatePointer());

	generateBuffer();

	changeEnvVar();

	printf("\nbtw my pid is %d\n", getpid());
	sleep(1000);
}

char* generatePointer() {
	char variable = 1;
	return &variable;
}

void generateBuffer() {
	char* buffer = malloc(100*sizeof(char));
	snprintf(buffer, 12, "hello world!");
	printf("Before free(): %s\n", buffer);
	free(buffer);
	printf("After free(): %s\n", buffer);

	char* newBuffer = malloc(100*sizeof(char));
	snprintf(buffer, 20, "another hello world!");
	printf("Before offsetting: %s\n", newBuffer);
	newBuffer += 50;
	printf("After offsetting: %s\n", newBuffer);
	free(newBuffer-50);
}

void changeEnvVar() {
	char *envVar = getenv("MYENVVAR");
	printf("envVar before updating: %s\n", envVar);

	setenv("MYENVVAR", "world!", 1);
	envVar = getenv("MYENVVAR");
	printf("envVar after updating: %s\n", envVar);
}