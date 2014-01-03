#ifndef CLIENT_H_
#define CLIENT_H_

#define SERVER_IP "168.192.122.31"
#define SERVER_PORT 9070

//显示命令
void show_command();

typedef struct{
	int client_socket;
	long have_send_data;
} send_data_t;

/**
 * @desc 读取文件内容
 * @param char * filepath 文件路径
 * @param int i 文件块索引
 * @param long * 文件块分组信息
 * @param char * chunk 返回的内存空间
 */
void readFileChunkContent(const char * filepath, int i, long *chunks_ptr, char *chunk);

#endif /* CLIENT_H_ */
