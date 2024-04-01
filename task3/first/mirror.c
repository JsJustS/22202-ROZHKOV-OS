#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define DIR_ENT struct dirent
#define ALL_PERMS S_IRWXU | S_IRWXO | S_IRWXG

void mirrorFile(const char* src, const char* dst);
void mirrorDir(const char* dirPath);
char* mirrorFilename(const char* filePath);
int isDir(const char* path);

void main(int argc, char** argv) {
	if (argc != 2) {
		printf("Use: %s /path/to/dir\n", argv[0]);
		return;
	}

	if (access(argv[1], F_OK) != 0) {
		printf("Error while accessing %s: %s\n", argv[1], strerror(errno));
		return;
	}

	if (!isDir(argv[1])) {
		printf("%s is not a directory!\n", argv[1]);
		return;
	}

	mirrorDir(argv[1]);
}

int isDir(const char* path) {
	struct stat fileStat;
	if (stat(path, &fileStat) != 0) {
		printf("Could not read stat data for %s: %s\n", path, strerror(errno));
		return 0;
	}
	return S_ISDIR(fileStat.st_mode);
}

void mirrorDir(const char* dirPath) {
	DIR* dirDesc = opendir(dirPath);
	DIR_ENT* dirEntry;

	// Создаём обратную пустую папку
	char* mirroredDirPath = mirrorFilename(dirPath);
	if (mkdir(mirroredDirPath, ALL_PERMS) != 0) {
		printf("Error while creating dir %s: %s\n", mirroredDirPath, strerror(errno));
		free(mirroredDirPath);
		return;
	}

	// Заполняем оригинальный и отражённый пути
	char filePath[100];
	char mirroredFilePath[100];
	int dirPathLen = strlen(dirPath);
	memcpy(filePath, dirPath, dirPathLen*sizeof(char));
	memcpy(mirroredFilePath, mirroredDirPath, dirPathLen*sizeof(char));
	filePath[dirPathLen] = '/';
	mirroredFilePath[dirPathLen] = '/';
	// Обходим оригинальную папку и копируем файлы
	while ((dirEntry = readdir(dirDesc)) != NULL) {
        	if (dirEntry->d_type == DT_REG) {
			// Скипаем temp файлы, их всё равно не видно в обычном режиме
			int nameLen = strlen(dirEntry->d_name);
			if (dirEntry->d_name[nameLen-1] == '~') {
				continue;
			}
			char* mirroredFileName = mirrorFilename(dirEntry->d_name);
			memcpy(filePath+dirPathLen+1, dirEntry->d_name, nameLen*sizeof(char));
			memcpy(mirroredFilePath+dirPathLen+1, mirroredFileName, nameLen*sizeof(char));
			free(mirroredFileName);
			filePath[dirPathLen+1+nameLen] = '\0';
			mirroredFilePath[dirPathLen+1+nameLen] = '\0';
			// debug: printf("==\n%s\n--\n%s\n", filePath, mirroredFilePath);
			// На этом этапе имеем оригинальный путь к файлу и отражённый путь к файлу
			mirrorFile(filePath, mirroredFilePath);
		}
	}

	free(mirroredDirPath);
	closedir(dirDesc);
}

char* mirrorFilename(const char* filepath) {
	int len = strlen(filepath);
	char* mirrored = malloc((len+1)*sizeof(char));
	mirrored[len] = '\0';

	// debug: printf("Path: %s\n", filepath);

	// Находим индекс начала имени файла в пути
	int i, j, k;
	j = -1;
	for (i = len; i > -1; --i) {
		if (filepath[i] == '.' && j == -1) {
			j = i;
		}
		if (filepath[i] == '/') {
			break;
		}
	}

	if (i != -1) {
		memcpy(mirrored, filepath, (i+1)*sizeof(char));
	}

	// debug: printf("After first if copy: %s\n", mirrored);
	if (j != -1) {
		memcpy(mirrored+j, filepath+j, (len-j)*sizeof(char));
	} else {
		j = len;
	}

	// debug: printf("After second if copy: %s\n", mirrored);
	for (k = 1; i + k < j ; ++k) {
		mirrored[i + k] = filepath[j - k];
	}

	// debug: printf("After mirroring: %s\n", mirrored);

	return mirrored;
}

#define BUFSIZE 1024
void mirrorFile(const char* src, const char* dst) {
	if (src == NULL || dst == NULL) {
		printf("Bad path provided\n");
		return;
	}

	struct stat srcStat;
	if (stat(src, &srcStat) != 0) {
		printf("Could not read data for %s: %s\n", src, strerror(errno));
		return;
	}
	int srcSize = srcStat.st_size;

	int srcDesc = open(src, O_RDWR);
	if (srcDesc == -1) {
		printf("Could not open %s: %s\n", src, strerror(errno));
		return;
	}

	if (lseek(srcDesc, (srcSize > BUFSIZE ? -BUFSIZE : -srcSize), SEEK_END) != 0) {
		close(srcDesc);
		printf("Could not set cursor for %s: %s\n", src, strerror(errno));
		return;
	}

	int dstDesc = open(dst, O_CREAT | O_RDWR | O_TRUNC, srcStat.st_mode);
	if (dstDesc == -1) {
		close(srcDesc);
		printf("Could not open %s: %s\n", src, strerror(errno));
		return;
	}

	unsigned char* buff = malloc(BUFSIZE*sizeof(char));
	unsigned char* mirrorBuff = malloc(BUFSIZE*sizeof(char));
	int count;
	while(srcSize > 0) {
		count = read(srcDesc, buff, BUFSIZE*sizeof(char));
		srcSize -= count;
		int i;
		for (i = 0; i < count; ++i) {
			mirrorBuff[i] = buff[count - 1 - i];
		}
		write(dstDesc, mirrorBuff, count);
		// Сдвигаем курсор влево на прочитанные сейчас байты + кусок для следующего чтения
		if (lseek(srcDesc, (srcSize > BUFSIZE ? -BUFSIZE : -srcSize) - count, SEEK_END) != 0) {
			close(srcDesc);
			close(dstDesc);
			free(buff);
			free(mirrorBuff);
			printf("Could not set cursor for %s: %s\n", src, strerror(errno));
			return;
		}
	}
	close(srcDesc);
	close(dstDesc);
	free(buff);
	free(mirrorBuff);
}