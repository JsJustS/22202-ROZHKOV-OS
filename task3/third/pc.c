#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

int readLine(int fd, char* buff, unsigned long long* offset);
int nextAddress(int fd, char* buff, unsigned long long* offset);
unsigned long long addressToLL(char* address);
void printPageMapping(int fd, unsigned long long vaddr);

unsigned long long getPFN(unsigned long long x);
int getBit(unsigned long long x, unsigned long long y);

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: %s <pid> [VADDR]\n", argv[0]);
		puts("\tVADDR - Vurtual Address from /proc/pid/maps");
		puts("\tOFFSET - Offset in pagemap file (in bytes)");
		puts("\tPGDATA - Data about Virtual Page with specified address (64 bits)");
		puts("\tPFN - Page Frame Number");
		return 1;
	}

	char* p = argv[1];
	for(;*p;++p) {
		if (!isdigit(*p)) {
			printf("Invalid pid provided: Non-digit string \"%s\"\n", argv[1]);
			return 1;
		}
	}

	// Парсим путь pagemap
	char path_p[100];
	sprintf(path_p, "/proc/%s/pagemap", argv[1]);
	printf("Printing data from \"%s\":\n", path_p);
	printf("VADDR\t\tOFFSET\t\tPGDATA\t\tPFN\n");
	
	int pfd;
	if ((pfd=open(path_p, O_RDONLY)) == -1) {
		fprintf(stderr, "Error while opening %s: %s\n", path_p, strerror(errno));
		return 1;
	}

	// Если передали один адрес - смотрим только его
	if (argc == 3) {
		unsigned long long vaddr = addressToLL(argv[2]);
		printPageMapping(pfd, vaddr);
		return 0;
	}

	// Иначе парсим /proc/pid/maps
	char path_m[100];
	sprintf(path_m, "/proc/%s/maps", argv[1]);
	int mfd;
	if ((mfd=open(path_m, O_RDONLY)) == -1) {
		close(pfd);
		fprintf(stderr, "Error while opening %s: %s\n", path_m, strerror(errno));
		return 1;
	}

	// Парсим
	int count;
	char address[9];
	address[8] = '\0';
	unsigned long long of = 0;
	while (nextAddress(mfd, address, &of) != -1) {
		unsigned long long vaddr = addressToLL(address);
		printPageMapping(pfd, vaddr);
	}

	// Закрываем
	close(mfd);
	close(pfd);
	return 0;
}

#define PAGE_ENTRY 8
void printPageMapping(int fd, unsigned long long vaddr) {
	printf("0x%llx\t", vaddr);

	unsigned long long pageSize = getpagesize();
	unsigned long long offset = vaddr / pageSize * PAGE_ENTRY;
	printf("%llu\t", offset);

	if (lseek(fd, offset, SEEK_SET)==-1) {
		fprintf(stderr, "\nError while offsetting for %llu bytes: %s\n", vaddr, strerror(errno));
		return;
	}
	
	unsigned char buff[PAGE_ENTRY];
	if (read(fd, buff, PAGE_ENTRY) == -1) {
		fprintf(stderr, "\nError while reading 0x%llx: %s\n", vaddr, strerror(errno));
		return;
	}

	unsigned long long result = 0;
	int i;
	for (i = 0; i < PAGE_ENTRY; ++i) {
		result = (result << 8) + buff[ (isBigendian()) ? i : PAGE_ENTRY-i-1 ];
	}
	printf("0x%llx\t", result);

	if (getBit(result, 63)) {
		unsigned long long pfn = getPFN(result);
		printf("0x%llx (0x%llx)\n", pfn, pfn * pageSize + vaddr % pageSize);
	} else {
		printf("Page not found\n");
	}
}

unsigned long long addressToLL(char* address) {
	unsigned long long value;
	sscanf(address, "%llx", &value);
	return value;
}

#define SIZE 100
int nextAddress(int fd, char* buff, unsigned long long* offset) {
	char line[SIZE];
	if (readLine(fd, line, offset) < 8) {
		//printf("Invalid address \"%s\"\n", line);
		return -1;
	}
	memcpy(buff, line, 8*sizeof(char));
	return 0;
}

int getBit(unsigned long long x, unsigned long long y) {
	return (x&(1<<y))>>y;
}

unsigned long long getPFN(unsigned long long x) {
	return x&((1ULL<<55)-1);
}

int isBigendian() {
	int x = 1;
	return (*(char*)&x)==0;
}

int readLine(int fd, char *buffer, unsigned long long* offset) {
	int bytesRead = 0;
	int i = 0;
	char *p = 0;

	// Сдвигаемся на оффсет по файлу
	if ((bytesRead = lseek(fd, *offset, SEEK_SET)) != -1) {
		// Если сдвинулись - читаем
		bytesRead = read(fd, buffer, SIZE*sizeof(char));
	}

	// Не сдвинулись или не прочитали
	if (bytesRead < 1) return -1;

	p = buffer;
	while (i < bytesRead && *p != '\n') ++p, ++i;
	*p = 0;

	// Если не нашли перенос строки
	if (i == bytesRead) {
		*offset += bytesRead;
		return bytesRead;
	}

	// Сдвигаем оффсет
	*offset += i + 1;

	return i;
}