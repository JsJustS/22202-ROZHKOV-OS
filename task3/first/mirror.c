#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

#define DIR_ENT struct dirent
#define ALL_PERMS S_IRWXU | S_IRWXO | S_IRWXG
#define BUFSIZE 1024

void mirrorFile(const char* src, const char* dst);
void mirrorDir(const char* dirPath, const char* mirroredDirPath);
void mirrorFilename(const char* filePath, char* buff);
int isDir(const char* path);

void main(int argc, char** argv) {
	if (argc != 2) {
		printf("Use: %s /path/to/dir\n", argv[0]);
		return;
	}

	if (access(argv[1], F_OK) != 0) {
		fprintf(stderr, "Error while accessing %s: %s\n", argv[1], strerror(errno));
		return;
	}

	if (!isDir(argv[1])) {
		fprintf("%s is not a directory!\n", argv[1]);
		return;
	}

	char* mirroredPath[strlen(argv[1])+1];
	mirrorFilename(argv[1], mirroredPath);
	mirrorDir(argv[1], mirroredPath);
}

int isDir(const char* path) {
	struct stat fileStat;
	if (stat(path, &fileStat) != 0) {
		fprintf(stderr, "Could not read stat data for %s: %s\n", path, strerror(errno));
		return 0;
	}
	return S_ISDIR(fileStat.st_mode);
}

void mirrorDir(const char* dirPath, const char* mirroredDirPath) {
	DIR* dirDesc = opendir(dirPath);
	DIR_ENT* dirEntry;

	// Создаём обратную пустую папку
	if (mkdir(mirroredDirPath, ALL_PERMS) != 0) {
		fprintf(stderr, "Error while creating dir %s: %s\n", mirroredDirPath, strerror(errno));
		return;
	}

	// Заполняем оригинальный и отражённый пути
	int dirPathLen = strlen(dirPath);
	char filePath[dirPathLen + 256];
	char mirroredFilePath[dirPathLen + 256];
	memcpy(filePath, dirPath, dirPathLen*sizeof(char));
	memcpy(mirroredFilePath, mirroredDirPath, dirPathLen*sizeof(char));
	filePath[dirPathLen] = '/';
	mirroredFilePath[dirPathLen] = '/';
	// Обходим оригинальную папку и копируем файлы
	while ((dirEntry = readdir(dirDesc)) != NULL) {
        	if (dirEntry->d_type != DT_REG && dirEntry->d_type != DT_DIR) {continue;}

		// Скипаем temp файлы, их всё равно не видно в обычном режиме
		int nameLen = strlen(dirEntry->d_name);
		if (dirEntry->d_name[nameLen-1] == '~' || strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0) {
			continue;
		}

		char* mirroredFileName[strlen(dirEntry->d_name)+1];
		mirrorFilename(dirEntry->d_name, mirroredFileName);
		memcpy(filePath+dirPathLen+1, dirEntry->d_name, nameLen*sizeof(char));
		memcpy(mirroredFilePath+dirPathLen+1, mirroredFileName, nameLen*sizeof(char));
		filePath[dirPathLen+1+nameLen] = '\0';
		mirroredFilePath[dirPathLen+1+nameLen] = '\0';

		// На этом этапе имеем оригинальный путь к файлу и отражённый путь к файлу
		if (dirEntry->d_type == DT_REG) {
			mirrorFile(filePath, mirroredFilePath);
		} else {
			mirrorDir(filePath, mirroredFilePath);
		}
	}

	closedir(dirDesc);
}

void mirrorFilename(const char* filepath, char* mirrored) {
	int len = strlen(filepath);
	mirrored[len] = '\0';

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

	if (j != -1) {
		memcpy(mirrored+j, filepath+j, (len-j)*sizeof(char));
	} else {
		j = len;
	}

	for (k = 1; i + k < j ; ++k) {
		mirrored[i + k] = filepath[j - k];
	}
}

void mirrorFile(const char* src, const char* dst) {
	if (src == NULL || dst == NULL) {
		fprintf(stderr, "Bad path provided\n");
		return;
	}

	struct stat srcStat;
	if (stat(src, &srcStat) != 0) {
		fprintf(stderr, "Could not read data for %s: %s\n", src, strerror(errno));
		return;
	}
	int srcSize = srcStat.st_size;

	int srcDesc = open(src, O_RDWR);
	if (srcDesc == -1) {
		fprintf(stderr, "Could not open %s: %s\n", src, strerror(errno));
		return;
	}

	if (lseek(srcDesc, (srcSize > BUFSIZE ? -BUFSIZE : -srcSize), SEEK_END) != 0) {
		close(srcDesc);
		fprintf(stderr, "Could not set cursor for %s: %s\n", src, strerror(errno));
		return;
	}

	int dstDesc = open(dst, O_CREAT | O_RDWR | O_TRUNC, srcStat.st_mode);
	if (dstDesc == -1) {
		close(srcDesc);
		fprintf(stderr, "Could not open %s: %s\n", src, strerror(errno));
		return;
	}

	char buff[BUFSIZE*sizeof(char)] = {0};
	char mirrorBuff[BUFSIZE*sizeof(char)] = {0};
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
			fprintf(stderr, "Could not set cursor for %s: %s\n", src, strerror(errno));
			return;
		}
	}
	close(srcDesc);
	close(dstDesc);
}