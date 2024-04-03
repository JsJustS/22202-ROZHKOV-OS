#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define DIR_ENT struct dirent
#define ALL_PERMS S_IRWXU | S_IRWXO | S_IRWXG

typedef enum Command {
	UNKNOWN_CMD,
	CREATE,
	CAT, // -p = permissions, -h = hardlinks count, 
	REMOVE,
	PERM
	
} Command;

typedef enum Argument {
	UNKNOWN_ARG,
	HELP,
	FILE,
	DIR_ARG,
	SYMLINK,
	HARDLINK,
	PERMISSION
} Argument;

void trunkName(char* name, const char* path);

int isDir(const char* path);
int isFile(const char* path);
int isLink(const char* path);

Command parseCommand(char* str);
int cmdCreate(int argc, char** argv);
int cmdCat(int argc, char** argv);
int cmdRemove(int argc, char** argv);
int cmdPermission(int argc, char** argv);

Argument parseArgument(char* arg);

void main(int argc, char** argv) {
	if (argc < 2) {
		printf("Use: %s /path/to/file [flags...]\n", argv[0]);
		return;
	}

	/*if (access(argv[1], F_OK) != 0) {
		printf("Error while accessing %s: %s\n", argv[1], strerror(errno));
		return;
	}*/
	char* cmd[100];
	trunkName(cmd, argv[0]);
	switch(parseCommand(cmd)) {
		case CREATE: return cmdCreate(argc, argv);
		case CAT: return cmdCat(argc, argv);
		case REMOVE: return cmdRemove(argc, argv);
		case PERM: return cmdPermission(argc, argv);
		default: printf("Unknown command: %s\n", argv[0]);
	}
	return 1;
}

Command parseCommand(char* str) {
	char* p = str;
	for (;*p;++p) {*p=tolower(*p);}

	if (strcmp(str, "create")==0) return CREATE;
	if (strcmp(str, "cat")==0) return CAT;
	if (strcmp(str, "remove")==0) return REMOVE;
	if (strcmp(str, "permission")==0) return PERM;
	return UNKNOWN_CMD;
}

Argument parseArgument(char* arg) {
	if (strcmp(arg, "-help")==0) return HELP;
	if (strcmp(arg, "-f")==0) return FILE;
	if (strcmp(arg, "-d")==0) return DIR_ARG;
	if (strcmp(arg, "-s")==0) return SYMLINK;
	if (strcmp(arg, "-h")==0) return HARDLINK;
	if (strcmp(arg, "-p")==0) return PERMISSION;
	return UNKNOWN_ARG;
}

void trunkName(char* name, const char* path) {
	int len = strlen(path);

	// Находим индекс начала имени файла в пути
	int i, k;
	for (i = len; i > -1; --i) {
		if (path[i] == '/') {
			break;
		}
	}
	// вставляем
	memcpy(name, path+i+1, (len-i)*sizeof(char));
}

int isDir(const char* path) {
	struct stat fileStat;
	if (lstat(path, &fileStat) != 0) {
		printf("Could not read stat data for %s: %s\n", path, strerror(errno));
		return 0;
	}
	return S_ISDIR(fileStat.st_mode);
}

int isFile(const char* path) {
	struct stat fileStat;
	if (lstat(path, &fileStat) != 0) {
		printf("Could not read stat data for %s: %s\n", path, strerror(errno));
		return 0;
	}
	return S_ISREG(fileStat.st_mode);
}

int isLink(const char* path) {
	struct stat fileStat;
	if (lstat(path, &fileStat) != 0) {
		printf("Could not read stat data for %s: %s\n", path, strerror(errno));
		return 0;
	}
	return S_ISLNK(fileStat.st_mode);
}

int cmdCreate(int argc, char** argv) {
	if (parseArgument(argv[1]) == HELP || argc < 3) {
		printf("Use: %s /path/to/file -flag <linkpath>\n", argv[0]);
		puts("\t-d\tcreates directory with specified path");
		puts("\t-f\tcreates file with specified path (all permissions granted)");
		puts("\t-s\tcreates symlink in current directory for specified path");
		puts("\t\tYou can specify second path where symlink will be put");
		puts("\t-h\tcreates hardlink in current directory for specified path");
		puts("\t\tYou can specify second path where hardlink will be put");
		return 0;
	}
	switch (parseArgument(argv[2])) {
		case DIR_ARG: {
			if (mkdir(argv[1], ALL_PERMS) != 0) {
				printf("Error while creating dir %s: %s\n", argv[1], strerror(errno));
				return 1;
			}
			break;
		}
		case FILE: {
			int fd = open(argv[1], O_CREAT | O_RDWR | O_TRUNC, ALL_PERMS);
    			if (fd == -1) {	
				printf("Error while creating file %s: %s\n", argv[1], strerror(errno));
				return 1;
    			} else {
				close(fd);
			}
			break;
		}
		case SYMLINK: {
			char* linkpath = (argc<4)?".":argv[3];
			if (symlink(argv[1], linkpath) == -1) {
				printf("Error while creating hardlink \"%s\" for file \"%s\": %s\n", linkpath, argv[1], strerror(errno));
				return 1;
    			}
			break;
		}
		case HARDLINK: {
			char* linkpath = (argc<4)?".":argv[3];
			if (link(argv[1], linkpath) == -1) {
				printf("Error while creating hardlink \"%s\" for file \"%s\": %s\n", linkpath, argv[1], strerror(errno));
				return 1;
    			}
			break;
		}
		default: {
			printf("Could not parse arguments for \"%s\"\n", argv[1]);
			return 1;
		}
	}
	return 0;
}

int cmdCat(int argc, char** argv) {
	if (parseArgument(argv[1]) == HELP || argc < 2 || argc > 3) {
		printf("Use: %s /path/to/file <-flag>\n", argv[0]);
		puts("\t\twithout flags prints data inside");
		puts("\t-s\tprints data inside symlink");		
		puts("\t-p\tprints file permissions");
		puts("\t-h\tprints hardlink count");
		return 0;
	}

	// print data
	if (argc == 2) {
		struct stat fileStat;
		if (stat(argv[1], &fileStat) != 0) {
			printf("Could not read stat data for %s: %s\n", argv[1], strerror(errno));
			return 1;
		}
		if (S_ISREG(fileStat.st_mode)) {
			// cat
			int fd = open(argv[1], O_RDONLY);
			if (fd == -1) {
				printf("Could not open %s: %s\n", argv[1], strerror(errno));
				return 1;
			}
			char buff[BUFSIZE];
			int count;
			while ((count = read(fd, buff, BUFSIZE)) > 0) {
				printf(buff);
    			}

			if (close(fd) == -1) {
				printf("Could not close %s: %s\n", argv[1], strerror(errno));
				return 1;
			}
			return 0;
		} else if (S_ISDIR(fileStat.st_mode)) {
			// ls
			DIR *dirDesc = opendir(argv[1]);
			DIR_ENT *entry;

			if (!dirDesc) {
				printf("Could not open %s: %s\n", argv[1], strerror(errno));
				return 1;
			}

			while ((entry = readdir(dirDesc)) != NULL) {
				puts(entry->d_name);
			}

			closedir(dirDesc);
			return 0;
		}
		puts("Specified file is not supported");
		return 1;
	} else {
		switch(parseArgument(argv[2])) {
			case SYMLINK: {
				char buff[BUFSIZE];
				int len = readlink(argv[1], buff, BUFSIZE-1);
				if (len == -1) {
					printf("Could not read symlink %s: %s\n", argv[1], strerror(errno));
					return 1;
				}
				buff[len] = '\0';
				printf("Symlink \"%s\" contains: %s\n", argv[1], buff);
				return 0;
			}
			case PERMISSION: {
				struct stat fileStat;
				if (stat(argv[1], &fileStat) != 0) {
					printf("Could not read stat data for %s: %s\n", argv[1], strerror(errno));
					return 1;
				}
				printf("File permissions for \"%s\": ", argv[1]);
				printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-"); // Проверяем, является ли файл директорией
				printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
				printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
				printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
				printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
				printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
				printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
				printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
				printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
				printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");
				printf("\n");
				return 0;
			}
			case HARDLINK: {
				struct stat fileStat;
				if (stat(argv[1], &fileStat) != 0) {
					printf("Could not read stat data for %s: %s\n", argv[1], strerror(errno));
					return 1;
				}
				printf("Hardlinks count for \"%s\": %d\n", argv[1], fileStat.st_nlink);
				return 0;
			}
		}
		printf("Wrong argument passed: \"%s\"\n", argv[2]);
	}
	return 1;
}

int cmdRemove(int argc, char** argv) {
	if (parseArgument(argv[1]) == HELP || argc != 2) {
		printf("Use: %s /path/to/file\n", argv[0]);
		puts("\t\t deletes entity specified in path");
		puts("\t\t if entity is a symlink - only this link will be deleted, original files won't change");
		return 0;
	}

	if (isLink(argv[1])) {
		if (unlink(argv[1])!=0) {
			printf("Error while removing symlink %s: %s\n", argv[1], strerror(errno));
			return 1;
		}
	}
	else if (isFile(argv[1])) {
		if (remove(argv[1])!=0) {
			printf("Error while removing file %s: %s\n", argv[1], strerror(errno));
			return 1;
		}
	}
	else if (isDir(argv[1])) {
		if (rmdir(argv[1])!=0) {
			printf("Error while removing dir %s: %s\n", argv[1], strerror(errno));
			return 1;
		}
	}
	return 0;
}

int cmdPermission(int argc, char** argv) {
	if (parseArgument(argv[1]) == HELP || argc != 3 || strlen(argv[2]) != 9) {
		printf("Use: %s /path/to/file p-e-r-m-s\n", argv[0]);
		puts("\t\t changes file's permissions to the ones provided by user");
		return 0;
	}

	int perms = 0;
	char *chars = "rwx";
	int i = 0;
	for (; i < 9; i++) {
		if (argv[2][i] == 'r') {
			if (i/3 == 0) {
				perms |= S_IRUSR;
			} else if (i/3 == 1) {
				perms |= S_IRGRP;
			} else {
				perms |= S_IROTH;
			}
		} else if (argv[2][i] == 'w') {
			if (i/3 == 0) {
				perms |= S_IWUSR;
			} else if (i/3 == 1) {
				perms |= S_IWGRP;
			} else {
				perms |= S_IWOTH;
			}
        	} else if (argv[2][i] == 'x') {
			if (i/3 == 0) {
				perms |= S_IXUSR;
			} else if (i/3 == 1) {
				perms |= S_IXGRP;
			} else {
				perms |= S_IXOTH;
			}
		} else if (argv[2][i] != '-') {
			printf("Invalid permission character: %c\n", argv[2][i]);
            		return 1;
		}
	}

	if (chmod(argv[1], perms) == -1) {
        	printf("Could not change permissions for %s: %s\n", argv[1], strerror(errno));
        	return 1;
    	}
	return 0;
}