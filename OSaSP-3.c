#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

const int MAXNAME = 255;

char* execFile;
int processLimit, processes;

int openDir(char*);
char* getFilePath(char*,char*);
int getProcess(char*);
int searchWords(char*);

int main(int argc, char* argv[])
{
	char *directory = argv[1];
	execFile = basename(argv[0]);
	processLimit = atoi(argv[2]);
	processes = 1;
	openDir(directory);
	while(processes != 0)
	{
		wait(NULL);
		processes--;
	}
	return 0;
}

char* getFilePath(char* path, char* name)
{
	char* fullPath = (char*)malloc(sizeof(char)*MAXNAME);
	strcpy(fullPath, path);
	strcat(fullPath, "/");
	strcat(fullPath, name);
	return fullPath;
}

int searchWords(char* filePath)
{
	FILE* file;
	char c;
	long int bytesRead = 0;
	int wordsRead = 0;
	
	file = fopen(filePath, "r");
	if (file == NULL)
	{
		fprintf(stderr, "%d : %s : %s : %s\n", getpid(), execFile, strerror(errno), filePath);
		return 1;
	}
	do
	{
		c=fgetc(file);
		bytesRead++;
		if (c!=' ' && c!='\n' && c!=EOF && c!='\t')
		{
			c = fgetc(file);
			bytesRead++;
			wordsRead++;
			while (c!=' ' && c!='\n'&& c!=EOF && c!='\t')
			{
				c=fgetc(file);
				bytesRead++;
			}
		}
	}
	while (c!=EOF);
	if (fclose(file) == EOF)
		fprintf(stderr, "%d : %s : %s : %s\n", getpid(), execFile, strerror(errno), filePath);
	printf("%d : %s : %ld bytes, %d words.\n", getpid(), filePath, bytesRead, wordsRead);
	return 0;
}

int getProcess(char* filePath)
{
	if (processes >= processLimit)
	{
		int result = wait(NULL);
		if (result > 0)
			processes--;
		if (result == -1)
		{
			fprintf(stderr, "%d %s: %s\n", getpid(), execFile, strerror(errno));
			return 1;
		}
	}
	if (processes < processLimit)
	{
		pid_t pid = fork();
		if (pid < 0)
		{
			fprintf(stderr, "%d : %s : %s\n", getpid(), execFile, strerror(errno));
			return 1;
		}
		if (pid == 0)
		{
			searchWords(filePath);
			exit(0); 			
		}
		if (pid > 0)
			processes++;
	}
	return 0;
}

int openDir(char* dirName)
{
	DIR* dir;
	struct dirent *readDir;
	struct stat buf;
	char* filePath;
	int tempError;
	dir = opendir(dirName);
 	if (dir == NULL)
	{
		fprintf(stderr, "%d : %s : %s : %s\n", getpid(), execFile, dirName, strerror(errno));
		return 1;
	}
	while((readDir = readdir(dir)) != NULL)
	{
		filePath=getFilePath(dirName, readDir->d_name);
		if ((readDir->d_type == DT_DIR) && (strcmp(readDir->d_name,".") != 0) && (strcmp(readDir->d_name,"..") != 0))
			openDir(filePath);
		else if (readDir->d_type == DT_REG)
			getProcess(filePath);		
		free(filePath);
	}
	if (errno == EBADF)
	{
		fprintf(stderr, "%d %s : %s\n", getpid(), execFile, strerror(errno));
		return 1;
	}
	if (closedir(dir) != 0)
	{
		fprintf(stderr, "%d %s : %s\n", getpid(), execFile, strerror(errno));
		return 1;
	}
	return 0;
}