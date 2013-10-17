#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "util.h"

//获取文件大小
long filesize(const char *filename)
{
	struct stat buf;
	if(stat(filename, &buf) < 0) {
		perror(str(errno));
	}
#ifdef DEBUG
	printf("file size: %ld\n", buf.st_size);
#endif
	return buf.st_size;
}


//反转字符串
void reverse(char *s)
{
  char *c;
  int i;
  c = s + strlen(s) - 1;
  while(s < c) {
    i = *s;
    *s++ = *c;
    *c-- = i;
  }
}

//帮助
void show_help(void)
{
	char *help = "only -h -c\n"
			"-h show help\n"
			"-c  config file path\n";
	fprintf(stderr, help, strlen(help));
}

//mem信息
void get_mem_info(mem_info_t *mem)
{
	FILE *fd;
	char buff[256];

	fd = fopen("/proc/meminfo", "r");
	fgets(buff, sizeof(buff), fd);
	//这个函数用的不错
	sscanf(buff, "%s %lu", mem->name, &mem->total);
	fgets(buff, sizeof(buff), fd);
	sscanf(buff, "%s %lu", mem->name2, &mem->free);

	fclose(fd);
}

//cpu信息
void get_cpu_info(cpu_info_t *cpu)
{
	FILE *fd;
	char buff[256];

	fd = fopen("/proc/stat", "r");
	fgets(buff, sizeof(buff), fd);
	sscanf(buff, "%s %u %u %u %u", cpu->name, &cpu->user, &cpu->nice, &cpu->system, &cpu->idle);


	fclose(fd);
}

//多个字符连接
void mult_strcat(char *buffer, char *format,...)
{
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
}

/**
 * 检测文件、目录是否存在    0表示存在    -1表示不存在
 */
int file_exists(const char *filepath)
{
	return access(filepath, F_OK);
}


//读取文件信息, 不验证文件是否存在
void file_get_contents(char *filepath, long filesize, char *content)
{
	int fileno = open(filepath, O_RDONLY);
	read(fileno, content, filesize);
	close(fileno);
}

//递归创建目录
int  recursive_create_dir(const char *sPathName)
{
  char DirName[256];
  strcpy(DirName, sPathName);
  int  i,len  =  strlen(DirName);
  if(DirName[len-1] != '/') {
	  strcat(DirName, "/");
  }

  len = strlen(DirName);

  for(i=1; i<len; i++){
	  if(DirName[i]=='/') {
		  DirName[i]   =   0;
		  if(access(DirName, F_OK) !=0) {
			  if(mkdir(DirName, 0777) == -1) {
                      perror("mkdir error");
                      return -1;
			  }
		  }
		  DirName[i]   =   '/';
	  }
  }
  return   0;
}


