#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../constant.h"
#include "../upload.h"
#include "../util.h"
#include "../md5.h"

#include "client.h"


//定义全局变量
const char filepath[] =  "client.c";


void signalHandler(int signalNo)
{
	printf("signal no: %d\n", signalNo);

	/*
	pid_t result = 0;
	int status = 0;
	result = waitpid(-1, &status, WNOHANG|WUNTRACED);//如果没有任何已经结束的子进程则马上返回，不予以等待。
	if(result < 0) {
		perror("signalHandler waitpid error");
		//exit(EXIT_FAILURE);
	}
	if (WIFEXITED(status)) { //如果子进程正常结束则为非0值
		printf("exited, status=%d/n", WEXITSTATUS(status)); //取得子进程exit()返回的结束代码，一般会先用WIFEXITED 来判断是否正常结束才能使用此宏
	} else if (WIFSIGNALED(status)) {
		printf("killed by signal %d/n", WTERMSIG(status));
	} else if (WIFSTOPPED(status)) {
		printf("stopped by signal %d/n", WSTOPSIG(status));
	} else if (WIFCONTINUED(status)) {
		printf("continued/n");
	}
	*/
}


int main(int argc, char **argv)
{
	int i = 0;

	//文件md5
	char file_md5[33];
	bzero(file_md5, sizeof(file_md5));
	md5_file(filepath, file_md5);

	//文件大小
	long file_size = filesize(filepath);

	//块大小
	long chunk_size = get_chunk_size();

	//块数量
	int chunk_nums = (int)ceil(file_size*1.0/chunk_size);

	long chunks[chunk_nums];
	bzero(chunks, sizeof(chunks));

	split_file_chunks(file_size, chunk_size, chunks);


	//和服务端打个招呼
	int clientSocket;
	struct sockaddr_in serverAddr;

	//创建个socket
	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket error");
		exit(EXIT_FAILURE);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	//连接服务端
	if(connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		perror("connect server error");
		exit(EXIT_FAILURE);
	}

	//发送命令
	char command[50];
	char  buffer[MAX_PACKET*2], chunk_meta_info[MAX_PACKET];

	memset(command, 0, 50);
	memset(buffer, 0, sizeof(buffer));
	memset(chunk_meta_info, 0, sizeof(chunk_meta_info));

	msg_t msg;
	bzero(&msg, sizeof(msg));
	socklen_t addrlen = sizeof(struct sockaddr_in);

	pid_t pids[chunk_nums], pid = 0, rtn = 0;
	send_data_t  *p_send_data;
	bzero(pids, sizeof(pids));

	srand((unsigned int)getpid());
	key_t ipc_shm_key = ftok("/dev/null", (int)'a');
	int shmid = 0;
	char chunk[chunk_size];
	bzero(chunk, sizeof(chunk));
	char *p;


	while(1) {
		printf("please input command:");
		scanf("%s", command);

		if(strcmp(command, "quit") == 0) {
			printf("quit!\n");
			exit(EXIT_SUCCESS);
		}
		switch(atoi(command)) {
			case SOCKET_TEST:
				//清空数据
				bzero(buffer, sizeof(buffer));

				msg.command = atoi(command);
				msg.fromTo = SOCKET_CLIENT;
				msg.sendTo = SOCKET_SERVER;
				printf("please test str:");
				msg.msgBody = (char *)calloc(100, 1);
				scanf("%s", msg.msgBody);
				msgtostr(&msg, buffer);
#ifdef DEBUG
				printf("buffer:%s\n", buffer);
#endif
				if(sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, addrlen) > 0) {
					if(recvfrom(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, &addrlen) > 0) {
						printf("recv data:%s\n", buffer);
						continue;
					} else {
						perror("recv from server data error");
						exit(EXIT_FAILURE);
					}
				} else {
					perror("send to server data error");
					exit(EXIT_FAILURE);
				}
			break;

			//查询数据已传输的情况
			case SOCKET_HAVE_SEND_DATA_QUERY:
				//清空数据
				bzero(buffer, sizeof(buffer));

				msg.command = atoi(command);
				msg.fromTo = SOCKET_CLIENT;
				msg.sendTo = SOCKET_SERVER;
				p = (char *)calloc(50, 1);
				mult_strcat(p, "%s_%ld", file_md5, file_size);
				msg.msgBody = p;
				msgtostr(&msg, buffer);

				if(sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, addrlen) > 0) {
					if(recvfrom(clientSocket, chunk_meta_info, sizeof(chunk_meta_info), 0, (struct sockaddr *)&serverAddr, &addrlen) > 0) {
						printf("recv data:%s\n", chunk_meta_info);
						sprintf(command, "%d", SOCKET_SEND_DATA);
						free(p);
						continue;
					} else {
						free(p);
						perror("recv from server data error");
						exit(EXIT_FAILURE);
					}
				} else {
					free(p);
					perror("send to server data error");
					exit(EXIT_FAILURE);
				}
			break;

			//传输数据
			case SOCKET_SEND_DATA:
				//关闭临时socket
				//close(clientSocket);
				shutdown(clientSocket, 2); //shutdown  close 区别 @see http://blog.csdn.net/jnu_simba/article/details/9068059

				//创建共享内存
				shmid = shmget(ipc_shm_key, sizeof(send_data_t)*chunk_nums, IPC_CREAT|0666);

				//因为要造出很多子进程，父进程比较忙, 处理先结束的进程，防止僵尸进程 所以 @see http://www.lupaworld.com/article-216737-1.html
				//signal(SIGCHLD, SIG_IGN); //通知内核，自己对子进程的结束不感兴趣，那么子进程结束后，内核会回收，并不再给父进程发送信号
				signal(SIGCHLD, signalHandler);
				//多进程传输数据
				for(i = 0; i < chunk_nums; i++) {
					pids[i] = pid = fork();
					if (pid == 0) {
						//共享空间连接到子进程的地址空间
						p_send_data = (send_data_t *)shmat(shmid, (void *)0, 0);

						//child process  传输数据
						printf("chunk:%d\n", i);
						//创建个socket
						clientSocket = socket(AF_INET, SOCK_STREAM, 0);
						if(clientSocket < 0) {
							perror("socket create");
							exit(1);
						}
						//连接服务端
						if(connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
								perror("connect server error");
								exit(1);
						}
						//发送数据
						msg.command = SOCKET_SEND_DATA;
						msg.fromTo = SOCKET_CLIENT;
						msg.sendTo = SOCKET_SERVER;
						readFileChunkContent(filepath, i, chunks, chunk);
						p = (char *)calloc(strlen(chunk) + 60, 1);
						mult_strcat(p, "%s_%ld_%d\n%s", file_md5, file_size, i, chunk);
						msg.msgBody = p;
						msgtostr(&msg, buffer);
						printf("send data:%s\n", buffer);
						if(sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAddr, addrlen) > 0) {
							//(p_send_data+i)->have_send_data = chunks[i];
							//分离共享空间
							shmdt(p_send_data);
							free(p);
							p = NULL;
							printf("end success\n");
							exit(EXIT_SUCCESS);
						} else {
							//分离共享空间
							shmdt(p_send_data);
							free(p);
							p = NULL;
							perror("send to server data error");
							exit(EXIT_FAILURE);
						}
						break;
					} else if (pid < 0) {
						perror("fork error");
						exit(1);
					}
				}
#ifdef DEBUG
				printf("fork child proccess num:%d\n", count(pids));
#endif
				while(1) {
					//等待所有子进程  会堵塞   如果在此之前的进程结束  这里会处理
					if((rtn = waitpid(-1, &rtn, 0)) > 0) {
						printf("child process %d closed\n", rtn);
					} else {
						perror("waitpid error");
						break;
					}
				}
				//删除共享内存
				/*
				if(shmctl(shmid, IPC_RMID, 0) < 0) {
					printf("shmctl error: %s\n", strerror(errno));
				}
				*/
				printf("%s", "data send complete");


			break;

			default:
				show_command();
			break;
		}
	}

	close(clientSocket);

	return 0;
}

//显示命令
void show_command()
{
	char command[] = "========================\n"
					 "1. command test\n"
					 "2. have send data query\n"
					 "========================\n";
	fprintf(stderr, command);
}

/**
 * @desc 读取文件内容
 * @param char * filepath 文件路径
 * @param int i 文件块索引
 * @param long * 文件块分组信息
 * @param char * chunk 返回的内存空间
 */
void readFileChunkContent(const char * filepath, int i, long *chunks_ptr, char *chunk)
{
	if(file_exists(filepath) < 0) {
		perror("file don't exists");
		exit(1);
	}
#ifdef DEBUG
	printf("%s filepath:%s\n", __FUNCTION__, filepath);
#endif
	int fd = open(filepath, O_RDONLY);
	long start = 0l;
	long idatanum = 0l;
	long chunk_size = get_chunk_size();
	start = chunk_size * i;
#ifdef DEBUG
	printf("chunk %d start pos %ld\n", i, start);
#endif
	if(lseek(fd, start, SEEK_SET) < 0) {
		perror("lseek error");
		close(fd);
		exit(1);
	}
	if((idatanum = read(fd, chunk, chunk_size)) < 0) {
		perror("read file error");
		close(fd);
		exit(1);
	}
#ifdef DEBUG
	printf("chunk content:%s\n", chunk);
#endif
	//重置文件读写位置
	lseek(fd, 0, SEEK_SET);
	close(fd);
}
