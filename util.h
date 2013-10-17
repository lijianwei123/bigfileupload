/*
 *  工具
 *  Created on: 2013-8-23
 *  Author: Administrator
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include<sys/types.h>
#include<fcntl.h>


//cpu状况
typedef struct
{
	char name[50];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
} cpu_info_t;

//mem状况
typedef struct
{
	char name[50];
	unsigned long total;
	char name2[50];
	unsigned long free;
} mem_info_t;


//转成字符串
#define str(s) #s

//计算数组长度
#define count(arr) (int)(sizeof(arr)/sizeof(arr[0]))

//获取文件大小  暂时不能计算windows挂载到linux的文件大小
long filesize(const char *);

//整形转字符串
//sprintf(char *, const char *format, ...);

//反转字符串
void reverse(char *);

//帮助信息
void show_help(void);

//mem信息
void get_mem_info(mem_info_t *);

//cpu信息
void get_cpu_info(cpu_info_t *);

//多个字符连接
void mult_strcat(char *, char *,...);


//检测文件、目录是否存在    0表示存在    -1表示不存在
int file_exists(const char *);


//读取文件信息, 不验证文件是否存在
void file_get_contents(char *filepath, long filesize, char *content);

//递归创建目录
int  recursive_create_dir(const char *sPathName);

#endif /* UTIL_H_ */
