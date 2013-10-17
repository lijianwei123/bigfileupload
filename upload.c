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
#include <unistd.h>
#include <sys/mman.h>


#include "upload.h"
#include "util.h"
#include "md5.h"
#include "constant.h"
#include "clockin.model.h"


client_sock_link_node_t *head, *client_sock_link_p;
pthread_mutex_t mutex;


//合并文件块
int combine_file_chunk(file_chunk_t *p_chunk, int chunk_size, const char *filepath)
{
	//定义
	FILE *fp;
	pthread_t threads[chunk_size];
	int index;
	pthread_mutex_t mut;
	chunk_param_t chunk_param;

	//初始化
	fp = fopen(filepath, "ab+");
	index = 0;
	pthread_mutex_init(&mut, NULL);


	while(p_chunk != NULL) {
		chunk_param.p_chunk = p_chunk;
		chunk_param.fp = fp;

		//开启多个线程写文件
		pthread_create(&threads[index], NULL, (void *)&write_chunk, (void *)&chunk_param);
		p_chunk++;
		index++;
	}
	for(index = 0; index < chunk_size; index++) {
		pthread_join(threads[index], NULL);
	}

	pthread_mutex_destroy(&mut);
	fclose(fp);

	return 0;
}

//写文件
void write_chunk(void *args)
{
	chunk_param_t *p_chunk_param = (chunk_param_t *)args;
	//文件定位
	int result = fseek(p_chunk_param->fp, p_chunk_param->p_chunk->offset, SEEK_SET);
	if(result == -1)
		perror("fseek");

	//开始写入
	result = fputs(p_chunk_param->p_chunk->chunk_content, p_chunk_param->fp);
	if(result == EOF)
		perror("fputs");

	pthread_exit(NULL);
}

//获取文件块大小  单位B
long get_chunk_size()
{
	return  11; //10KB
}

long get_filesize()
{
	return filesize("/home/test/lijianwei/squid-3.1.19.tar.gz");
}

/**
 * @desc分割文件
 * @param long filesize 总的文件大小,单位B
 * @param long chunk_size 块大小 ,单位B
 * @param long * p_chunks 块数组
 * @return long *
 */
long * split_file_chunks(long filesize, long chunk_size, long * p_chunks)
{
	int chunk_nums = (int)ceil(filesize*1.0/chunk_size);
	memset(p_chunks, 0, sizeof(p_chunks));

	int i = 0;
	for(i = 0; i < chunk_nums - 1; i++) {
		*(p_chunks + i) = chunk_size;
	}
	*(p_chunks + i) = filesize - (chunk_nums - 1) * chunk_size;
#ifdef DEBUG
	printf("file %d chunks info:\n", chunk_nums);
	for(i = 0; i < chunk_nums; i++) {
		printf("chunk %d size: %ld\n", i, *(p_chunks + i));
	}
#endif
	return p_chunks;
}


/*
 * 	文件块元数据  记录总的文件块信息，顺序
 * 	信息格式
 *
 * 	块编号 块已传输pos
 *
 *	块编号  文件md5_index
 *
*/
void save_chunk_meta(pthread_mutex_t *p_mutex, char *md5_str, chunk_meta_t *metas, int meta_num)
{
	pthread_mutex_lock(p_mutex);

	char filepath[34];
	bzero(filepath, sizeof(filepath));

	getChunkMetaFile(md5_str, filepath);

	char line[512];
	bzero(line, 512);
	int i = 0;

	//保存信息
	FILE *fp = fopen(filepath, "ab+");
	while(i < meta_num) {
		mult_strcat(line, "%d %d %d%s", metas->chunk_no, metas->offset, metas->have_send_pos, "\n");
		fputs(line, fp);
		metas += i;
		i++;
	}
	fclose(fp);

	pthread_mutex_unlock(p_mutex);
}

/*
 *	根据文件md5找到文件块信息文件路径
 *  md5_str bedfcd753054804642d28389df80dfe3
 *	filepath be/df/bedfcd753054804642d28389df80dfe3
 */
void getChunkMetaFile(char *md5_str, char *filepath)
{
	memset(filepath, 0, sizeof(filepath));

	*(filepath++) = *(md5_str++);
	*(filepath++) = *(md5_str++);
	*(filepath++) = '/';
	*(filepath++) = *(md5_str++);
	*(filepath++) = *(md5_str++);
	*(filepath++) = '/';
	md5_str -= 4;
	strcpy(filepath, md5_str);
}

//初始化文件块信息文件
void initChunkMetaFile(char *chunk_meta_filepath, long filesize, char *md5_str)
{
	long chunk_size = get_chunk_size();
	int chunk_nums = (int)ceil(filesize*1.0/chunk_size);

	//创建目录
	char *p;
	if((p = strrchr(chunk_meta_filepath, '/')) != 0) {
		int dir_len = p - chunk_meta_filepath;
		char dir[dir_len+1];
		bzero(dir, sizeof(dir));
		strncpy(dir, chunk_meta_filepath, dir_len);
		recursive_create_dir(dir);
	}
	int fileno = open(chunk_meta_filepath, O_RDWR|O_TRUNC|O_CREAT, 0777); //创建文件  可读写   文件权限 0777 & ~022 umask
	int i = 0;

	//32_0000_00000000
	//md5_块编号_已上传大小
	char line_str[50];
	bzero(line_str, sizeof(line_str));
	for(i = 0; i < chunk_nums; i++) {
		sprintf(line_str, "%s_%04d_%08d", md5_str, i, 0);
		write(fileno, line_str, strlen(line_str));
		write(fileno, "\n", 1);
	}
	close(fileno);
}

/**
 *	获取存储块文件路径
 *	chunk_md5_extra_ptr   bedfcd753054804642d28389df80dfe3_filesize_chunkindex
 *	filepath   be/df/bedfcd753054804642d28389df80dfe3_filesize_chunkindex
 */
void getStoreChunkFile(char *chunk_md5_extra_ptr, char *filepath)
{
	char md5_str[33];
	bzero(md5_str, sizeof(md5_str));


	sscanf(chunk_md5_extra_ptr, "%s_", md5_str);

	char *p1 = NULL, *p2 = NULL;
	p1 = filepath;
	p2 = md5_str;

	*(p1++) = *(p2++);
	*(p1++) = *(p2++);
	*(p1++) = '/';
	*(p1++) = *(p2++);
	*(p1++) = *(p2++);
	*(p1++) = '/';
	strcpy(p1, chunk_md5_extra_ptr);
}


//str转message
void strtomsg(char *str, msg_t *msg_ptr)
{
	cJSON *json;

	json = cJSON_Parse(str);
	if(!json) {
		printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		exit(EXIT_FAILURE);
	}
	if(json->type == cJSON_Object){
		msg_ptr->command = json->child->valueint;

		msg_ptr->fromTo = (char *)calloc(strlen(json->child->next->valuestring)+1, 1);
		memcpy(msg_ptr->fromTo, json->child->next->valuestring, strlen(json->child->next->valuestring)+1);

		msg_ptr->sendTo = (char *)calloc(strlen(json->child->next->next->valuestring)+1, 1);
		memcpy(msg_ptr->sendTo, json->child->next->next->valuestring, strlen(json->child->next->next->valuestring)+1);

		msg_ptr->msgBody = (char *)calloc(strlen(json->child->next->next->next->valuestring)+1, 1);
		memcpy(msg_ptr->msgBody, json->child->next->next->next->valuestring, strlen(json->child->next->next->next->valuestring)+1);

		cJSON_Delete(json);
	}
}

//msg to str
void msgtostr(msg_t *msg_ptr, char *str)
{
	cJSON *tmp = cJSON_CreateObject();
	char *out;
	cJSON_AddNumberToObject(tmp, "command", msg_ptr->command);
	cJSON_AddStringToObject(tmp, "fromTo", msg_ptr->fromTo);
	cJSON_AddStringToObject(tmp, "sendTo", msg_ptr->sendTo);
	cJSON_AddStringToObject(tmp, "msgBody", msg_ptr->msgBody);
	out = cJSON_PrintUnformatted(tmp);
	memcpy(str, out, strlen(out) + 1);
	cJSON_Delete(tmp);
}


void free_client_socket_param(client_socket_param_t *param)
{
	close(*(param->clientSocket));
	free(param->clientSocket);
	param->clientSocket = NULL;
	free(param->clientIp);
	param->clientIp = NULL;
	free(param->clientPort);
	param->clientPort = NULL;
	free(param);
	param = NULL;
}


void free_part_msg(msg_t *msg)
{
	free(msg->fromTo);
	msg->fromTo = NULL;  //避免野指针
	free(msg->sendTo);
	msg->sendTo = NULL;
	free(msg->msgBody);
	msg->msgBody = NULL;
	//free(msg); //不是通过malloc  calloc申请的空间不可以用free释放
	//msg = NULL;
}

void *processthread(void *para)
{
	client_socket_param_t *p_socket_param;

	p_socket_param = (client_socket_param_t *)para;

	char recvdata[MAX_PACKET]; //recv data
	char senddata[MAX_PACKET]; //send data
	int idatanum;

	int clientsocket = *(p_socket_param->clientSocket);

	msg_t recv_msg, send_msg; //数据格式

	while(1){
		//初始化
		memset(recvdata, 0, MAX_PACKET);
		memset(senddata, 0, MAX_PACKET);
		memset(&recv_msg, 0, sizeof(recv_msg));
		memset(&send_msg, 0, sizeof(send_msg));

		//接受数据
		idatanum = recv(clientsocket, recvdata, MAX_PACKET, 0);
#ifdef DEBUG
		printf("recv data: %s, idatanum: %d\n", recvdata, idatanum);
#endif
		if(idatanum <= 0){
			perror("recv data error");
			break;
		}

		strtomsg(recvdata, &recv_msg); //解析数据

		switch(recv_msg.command){
			//获取服务器状态
			case SOCKET_SERVERINFO:
				processServerInfo(&send_msg, p_socket_param);
			break;

			//打卡
			case SOCKET_CLOCKIN:
				processClockIn(&recv_msg, p_socket_param, &send_msg);
			break;

			//测试，直接返回
			case SOCKET_TEST:
				processTest(&recv_msg, p_socket_param, &send_msg);
			break;

			//查询已经发送数据的情况
			case SOCKET_HAVE_SEND_DATA_QUERY:
				processHaveSendDataQuery(&recv_msg, p_socket_param, &send_msg);
			break;

			//接受数据
			case SOCKET_SEND_DATA:
				processRecvData(&recv_msg, p_socket_param, &send_msg);
			break;
		}

		msgtostr(&send_msg, senddata);

#ifdef DEBUG
		printf("senddata:%s\n", senddata);
#endif
		int error = send(*(p_socket_param->clientSocket), senddata, strlen(senddata) + 1, 0);
		if(error <= 0){
			perror("send error");

			free_part_msg(&recv_msg);
			free_part_msg(&send_msg);

			break;
		}

		free_part_msg(&recv_msg);
		free_part_msg(&send_msg);
	}
	printf("client:%s %hu closed!\n", p_socket_param->clientIp, *(p_socket_param->clientPort));
	//删除单向socket链表
	pthread_mutex_lock(&mutex);
	del_client_sock_node(head, *(p_socket_param->clientSocket));
	pthread_mutex_unlock(&mutex);
	//清除内存资源
	free_client_socket_param(p_socket_param);
	//关闭线程
	pthread_exit(NULL);
	pthread_detach(pthread_self()); //解决僵尸线程  不释放内存问题  @see http://www.cnblogs.com/bits/archive/2009/12/04/no_join_or_detach_memory_leak.html
}

//服务器信息
void processServerInfo(msg_t *send_msg_ptr, client_socket_param_t *socket_param_ptr)
{
	mem_info_t mem;
	cpu_info_t cpu;
	char format[20] = {0};
	char *p1, *p2;
	unsigned short *p3;

	get_cpu_info(&cpu);
	get_mem_info(&mem);

	send_msg_ptr->msgBody = (char *)calloc(MAX_BODY_PACKET, 1);
	sprintf(format, "%s%s%s", "%s", SOCKET_MSG_SPLIT, "%s");
	p1 = str(cpu.idle);
	p2 = str(mem.free);
	mult_strcat(send_msg_ptr->msgBody, format, p1, p2);
	free(p1); p1 = NULL; free(p2); p2 = NULL;

	send_msg_ptr->command = SOCKET_ANONY;
	send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
	strcpy(send_msg_ptr->fromTo,SOCKET_SERVER);
	send_msg_ptr->sendTo = (char *)calloc(MAX_BODY_PACKET, 1);
	memset(format, 0, 20);
	sprintf(format, "%s%s%s", "%s", SOCKET_MSG_SPLIT, "%d");
	mult_strcat(send_msg_ptr->sendTo, format, p1 = socket_param_ptr->clientIp, p3 = socket_param_ptr->clientPort);

	free(p1); p1 = NULL; free(p3); p3 = NULL;
}

//打卡操作
void processClockIn(msg_t *recv_msg_ptr, client_socket_param_t *socket_param_ptr, msg_t *send_msg_ptr)
{
	//解析数据
	char *fromTo, *phoneNumber, *phoneMac, *s, *datetime;
	int result;

	fromTo = recv_msg_ptr->fromTo;
	if(!strchr(fromTo, atoi(SOCKET_MSG_SPLIT))){ //如果信息没有用分隔符
		send_msg_ptr->command = SOCKET_CLOCKIN_RETURN;
		send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
		strcpy(send_msg_ptr->fromTo, SOCKET_SERVER);
		send_msg_ptr->sendTo = NULL;
		s = str(RETURN_FAILURE);
		send_msg_ptr->msgBody = (char *)calloc(sizeof(s), 1);
		strcpy(send_msg_ptr->msgBody, s);
		free(s); s = NULL;
		return;
	}

	phoneNumber = strtok(fromTo, SOCKET_MSG_SPLIT);
	phoneMac = strtok(NULL, SOCKET_MSG_SPLIT);
	datetime = str(time(0)); // NULL  == 0  time(NULL)  time_t  long
	//调用mysql 插入数据
	result = insert_clockin(phoneNumber, phoneMac, datetime);
	if(result < 0){
		send_msg_ptr->command = SOCKET_CLOCKIN_RETURN;
		send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
		strcpy(send_msg_ptr->fromTo, SOCKET_SERVER);
		send_msg_ptr->sendTo = NULL;
		s = str(RETURN_FAILURE);
		send_msg_ptr->msgBody = (char *)calloc(sizeof(s), 1);
		strcpy(send_msg_ptr->msgBody, s);
		free(s); s = NULL;
		return;
	}
	//成功
	send_msg_ptr->command = SOCKET_CLOCKIN_RETURN;
	send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
	strcpy(send_msg_ptr->fromTo, SOCKET_SERVER);
	send_msg_ptr->sendTo = NULL;
	s = str(RETURN_SUCCESS);
	send_msg_ptr->msgBody = (char *)calloc(sizeof(s), 1);
	strcpy(send_msg_ptr->msgBody, s);

	free(datetime); datetime = NULL;
	return;
}


//测试，直接返回
void processTest(msg_t  *recv_msg_ptr, client_socket_param_t *socket_param_ptr, msg_t *send_msg_ptr)
{
	char buffer[512];
	memset(buffer, 0, 512);

	send_msg_ptr->command = SOCKET_TEST;
	send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
	strcpy(send_msg_ptr->fromTo, SOCKET_SERVER);

	mult_strcat(buffer, "%s %hu", socket_param_ptr->clientIp, socket_param_ptr->clientPort);
	send_msg_ptr->sendTo = (char *)calloc(strlen(buffer)+1, 1);
	strcpy(send_msg_ptr->sendTo, buffer);

	send_msg_ptr->msgBody = (char *)calloc(strlen(recv_msg_ptr->msgBody)+1, 1);
	strcpy(send_msg_ptr->msgBody, recv_msg_ptr->msgBody);
}

/**
 * 查询已经发送数据的情况
 * recv_msg_ptr->msgBody = "404332cf52bca1ed66f60627f161032f"
 */
void processHaveSendDataQuery(msg_t *recv_msg_ptr, client_socket_param_t *socket_param_ptr, msg_t *send_msg_ptr)
{
	char buffer[512], md5_str[33], chunk_meta_file[39];
	long file_size = 0l;
	memset(buffer, 0, sizeof(buffer));
	memset(md5_str, 0, sizeof(md5_str));
	memset(chunk_meta_file, 0, sizeof(chunk_meta_file));

	send_msg_ptr->command =	SOCKET_HAVE_SEND_DATA_QUERY;
	send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
	strcpy(send_msg_ptr->fromTo, SOCKET_SERVER);

	mult_strcat(buffer, "%s %hu", socket_param_ptr->clientIp, socket_param_ptr->clientPort);
	send_msg_ptr->sendTo = (char *)calloc(strlen(buffer)+1, 1);
	strcpy(send_msg_ptr->sendTo, buffer);

	sscanf(recv_msg_ptr->msgBody, "%32s_%ld", md5_str, &file_size); //只取32个字符

	//验证md5_str 长度是否符合
	if(strlen(md5_str) != 32) {
		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer, "error:md5 length != 32");
		send_msg_ptr->msgBody = (char *)calloc(strlen(buffer)+1, 1);
		strcpy(send_msg_ptr->msgBody, buffer);
		return;
	}


	//获取meta信息
	getChunkMetaFile(md5_str, chunk_meta_file);
	if(file_exists(chunk_meta_file) < 0) {
		initChunkMetaFile(chunk_meta_file, file_size, md5_str);
	}
	long meta_file_size = filesize(chunk_meta_file);
	char content[meta_file_size];
	bzero(content, sizeof(content));
	file_get_contents(chunk_meta_file, meta_file_size - 1, content);

	send_msg_ptr->msgBody = (char *)calloc(strlen(content)+1, 1);
	strcpy(send_msg_ptr->msgBody, content);
}

/*
 * 接受数据
 * recv_msg_ptr->msgBody
 * 404332cf52bca1ed66f60627f161032f_111\n文件块内容
 */
void processRecvData(msg_t *recv_msg_ptr, client_socket_param_t *socket_param_ptr, msg_t *send_msg_ptr)
{
	//初始化发送信息数据
	send_msg_ptr->command =	SOCKET_SEND_DATA;
	send_msg_ptr->fromTo = (char *)calloc(sizeof(SOCKET_SERVER), 1);
	strcpy(send_msg_ptr->fromTo, SOCKET_SERVER);

	char buffer[512];
	mult_strcat(buffer, "%s %hu", socket_param_ptr->clientIp, socket_param_ptr->clientPort);
	send_msg_ptr->sendTo = (char *)calloc(strlen(buffer)+1, 1);
	strcpy(send_msg_ptr->sendTo, buffer);

	send_msg_ptr->msgBody = (char *)calloc(strlen("success")+1, 1);
	strcpy(send_msg_ptr->msgBody, "success");

	//获取文件块路径
	char line_break = '\n';
	char *allcontent = recv_msg_ptr->msgBody;
	char *first_line_break = strchr(allcontent, line_break);
	int md5_extra_str_len = first_line_break - allcontent;
	md5_extra_str_len++;
	char md5_extra_str[md5_extra_str_len];
	bzero(md5_extra_str, md5_extra_str_len);
	strncpy(md5_extra_str, allcontent, --md5_extra_str_len);

	//文件块内容
	allcontent += md5_extra_str_len;
	allcontent++;

	//文件块路径
	int store_chunk_path_len = 6 + sizeof(md5_extra_str);
	char filepath[store_chunk_path_len];
	getStoreChunkFile(md5_extra_str, filepath);


	//写入文件
	FILE *fp = fopen(filepath, "wb");
	fputs(allcontent, fp);
	fclose(fp);

	//获取文件块信息文件
	char md5_str[33], chunk_meta_filepath[39];
	int chunk_index;
	long file_size;
	bzero(md5_str, sizeof(md5_str));
	bzero(chunk_meta_filepath, sizeof(chunk_meta_filepath));

	sscanf(md5_extra_str, "%32s_%ld_%d", md5_str, &file_size, &chunk_index); //md5_str
	getChunkMetaFile(md5_str, chunk_meta_filepath); //文件块信息文件

	if(file_exists(chunk_meta_filepath) < 0) {
		initChunkMetaFile(chunk_meta_filepath, file_size, md5_str);
	}

	//使用mmap映射到内存  对内存的修改也会同步修改文件内容
	int fileno = open(chunk_meta_filepath, O_RDWR);
	if(flock(fileno, LOCK_EX) == 0) {
		struct stat st;
		fstat(fileno, &st);
		char *mmaped, *orgin;
		orgin = mmaped = (char *)mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fileno, 0);
		close(fileno);
		if(mmaped == MAP_FAILED) {
			perror("mmap error");
			exit(EXIT_FAILURE);
		}


		mmaped += (chunk_index) * 47;
		mmaped += 38;

		char chunk_size[8];
		bzero(chunk_size, sizeof(chunk_size));
		sprintf(chunk_size, "%08ld", get_chunk_size());

		memcpy(mmaped, chunk_size, 8);

		//解除mmap映射
		munmap(orgin, st.st_size);

		flock(fileno, LOCK_UN);
	}
}


//删除链表
void del_client_sock_node(client_sock_link_node_t *head, int clientSocket)
{
	client_sock_link_node_t *p1, *p2;
	p1 = head->next;
	p2 = head;
	while(p1->next != NULL) {
		if(*(p1->client_socket_ptr) == clientSocket) {
			p2->next = p1->next;
			free(p1);
			break;
		}
		p2 = p1;
		p1 = p1->next;
	}
}

