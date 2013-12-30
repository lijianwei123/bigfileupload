#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#include "upload.h"
#include "util.h"
#include "md5.h"
#include "constant.h"
#include "clockin.model.h"

void signalHandler(int signalNo)
{
	printf("signal no: %d\n", signalNo);
	//循环单向链表  关闭客户端
	client_sock_link_node_t *p;
	p = head->next;
	while(p->next != NULL) {
		close(*(p->client_socket_ptr));
		p = p->next;
	}

	extern int serverSocket; //extern在这里是扩大42行定义的serverSocket范围

	//关闭服务端
	close(serverSocket);

	//销毁线程锁
	pthread_mutex_destroy(&mutex);

	exit(0);
}


int serverSocket;

int main(int argc, char **argv)
{
	//注册信号处理函数
	signal(SIGINT, signalHandler);  //ctrl + c
	signal(SIGTERM, signalHandler); //kill pkill  要求程序正常退出  本信号能被阻塞、处理和忽略
	//signal(SIGKILL, signalHandler); //kill -9  本信号不能被阻塞、处理和忽略
	signal(SIGCHLD, SIG_IGN);//SIGCHLD  子进程正常或异常终止的时候，内核就向其父进程发送SIGCHLD信号   SIG_IGN 忽略信号  SIG_DFL  核心预设的信号处理方式
	signal(SIGQUIT, SIG_IGN);//SIGQUIT  ctrl + \ 默认的处理方式是直接退出    会产生 core dumped 文件
	signal(SIGSEGV, signalHandler);
	//sigaction  @see  http://blog.csdn.net/jiang1013nan/article/details/5409684


	head = (client_sock_link_node_t *)calloc(sizeof(client_sock_link_node_t), 1);
	client_sock_link_p = head;

	pthread_mutex_init(&mutex, NULL);

	int bind_port = 9070;


	struct sockaddr_in server_addr, client_addr;
	socklen_t socklen = sizeof(server_addr);

	//创建socket
	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("serversocket error");
		exit(EXIT_FAILURE);
	}

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(bind_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定
	if(bind(serverSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	//监听
	if(listen(serverSocket, 500) < 0) {
			perror("listen error");
			exit(EXIT_FAILURE);
	}

	char *p = NULL;
	client_socket_param_t *p_socket_param;
	while(1) {
			p_socket_param = (client_socket_param_t *)calloc(sizeof(client_socket_param_t), 1);
			p_socket_param->clientSocket = (int *)calloc(sizeof(int), 1);

			//接受请求
			*(p_socket_param->clientSocket) = accept(serverSocket, (struct sockaddr *)&client_addr, (socklen_t *)&socklen);

			if(*(p_socket_param->clientSocket) < 0) {
				free(p_socket_param->clientSocket);
				p_socket_param->clientSocket = NULL;
				free(p_socket_param);
				p_socket_param = NULL;
				perror("accept error");
				continue;
			}
			//记录到客户端单向链表中
			client_sock_link_p->next = (client_sock_link_node_t *)calloc(sizeof(client_sock_link_node_t), 1);
			client_sock_link_p = client_sock_link_p->next;
			client_sock_link_p->client_socket_ptr = p_socket_param->clientSocket;

			/*
			 * @see 这段代码写的非常好   http://blog.csdn.net/ipromiseu/article/details/4165383
			 * 记录请求的ip 和  端口
			 */
			p = inet_ntoa(client_addr.sin_addr);
			p_socket_param->clientIp = (char *)calloc(strlen(p)+1, 1);
			strcpy(p_socket_param->clientIp, p);
			p = NULL;

			p_socket_param->clientPort = (unsigned short *)calloc(sizeof(unsigned short), 1);
			*(p_socket_param->clientPort) = ntohs(client_addr.sin_port);


			//创建线程   处理具体业务
			pthread_t threadid;
			int temp = pthread_create(&threadid, NULL, processthread, (void *)p_socket_param);
			if(temp != 0) {
				perror("create thread error");
				exit(EXIT_FAILURE);
			}

#ifdef DEBUG
			printf("threadid:%lu\n", threadid);
			printf("clientIp:%s\n", p_socket_param->clientIp);
			printf("clientPort:%hu\n", *(p_socket_param->clientPort));
#endif

	}

	close(serverSocket);
	return 0;
}
