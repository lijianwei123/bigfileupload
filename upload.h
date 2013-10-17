/*
 * upload.h
 *
 *  Created on: 2013-8-22
 *  Author: lijianwei
 */
#include <stddef.h>
#include <fcntl.h>
#include<sys/file.h>

#include "cJSON.h"

#ifndef UPLOAD_H_
#define UPLOAD_H_

//文件块结构
typedef struct{
	char *chunk_content; //文件块内容
	size_t offset; //写入位置
} file_chunk_t;

//文件块 参数
typedef struct{
	FILE *fp;
	file_chunk_t *p_chunk;
} chunk_param_t;

//文件块元数据
typedef struct{
	 long chunk_no; //编号
	 long offset; //pos
	 long have_send_pos;
} chunk_meta_t;

//客户端socket结构
typedef struct{
	int *clientSocket;
	char *clientIp;
	unsigned short *clientPort;

} client_socket_param_t;

//信息结构
typedef struct
{
	int command;
	char *fromTo;
	char *sendTo;
	char *msgBody;
} msg_t;

//客户端socket单向链表
typedef struct client_sock_link_node
{
	int *client_socket_ptr;
	struct client_sock_link_node *next;
} client_sock_link_node_t;


extern pthread_mutex_t mutex;
extern client_sock_link_node_t *head;
extern client_sock_link_node_t *client_sock_link_p;


//删除链表
void del_client_sock_node(client_sock_link_node_t *head, int clientSocket);


//合并文件块
int combine_file_chunk(file_chunk_t *, int, const char *);

//写文件块
void write_chunk(void *);

//获取文件块大小  单位B
long get_chunk_size();

//获取文件大小  单位B
long get_filesize();

/**
 * @desc分割文件
 * @param long filesize 总的文件大小,单位B
 * @param long chunk_size 块大小 ,单位B
 * @param long * p_chunks 块数组
 * @return long *
 */
long * split_file_chunks(long filesize, long chunk_size, long * p_chunks);

//创建文件块元数据    实现断点续传
void create_file_chunks_meta();

//更新文件块      实现断点续传
void update_file_chunk_meta();

/*
 *	根据文件md5找到文件块信息文件路径
 *  md5_str bedfcd753054804642d28389df80dfe3
 *	filepath be/df/bedfcd753054804642d28389df80dfe3
 */
void getChunkMetaFile(char *md5_str, char *filepath);

/**
 *	获取存储块文件路径
 *	chunk_md5_extra_ptr   bedfcd753054804642d28389df80dfe3_111
 *	filepath   be/df/bedfcd753054804642d28389df80dfe3_111
 */
void getStoreChunkFile(char *chunk_md5_extra_ptr, char *filepath);

//初始化文件块信息文件
void initChunkMetaFile(char *chunk_meta_filepath, long filesize, char *md5_str);

//str转message   解码信息
void strtomsg(char *, msg_t *);

//msg to str     编码信息
void msgtostr(msg_t *, char *);

//释放client_socket_param_t 内存
void free_client_socket_param(client_socket_param_t *param);

//释放msg_t 内存
void free_part_msg(msg_t *);

//处理具体业务
void *processthread(void *);

//获取服务器运行信息
void processServerInfo(msg_t *, client_socket_param_t *);

//打卡操作
void processClockIn(msg_t *, client_socket_param_t *, msg_t *);

//测试
void processTest(msg_t *, client_socket_param_t *, msg_t *);

//查询已经发送数据的情况
void processHaveSendDataQuery(msg_t *recv_msg_ptr, client_socket_param_t *socket_param_ptr, msg_t *send_msg_ptr);

//接受数据
void processRecvData(msg_t *recv_msg_ptr, client_socket_param_t *socket_param_ptr, msg_t *send_msg_ptr);

#endif /* UPLOAD_H_ */
