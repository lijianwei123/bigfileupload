/*
 * constant.h
 *
 *  Created on: 2013-8-28
 *      Author: Administrator
 */

#ifndef CONSTANT_H_
#define CONSTANT_H_

//通讯最大数据包 10K
#define MAX_PACKET 10240
#define MAX_BODY_PACKET 10000
#define SOCKET_MSG_SPLIT " "
#define SOCKET_SERVER "SERVER"
#define SOCKET_CLIENT "CLIENT"

typedef enum {RETURN_SUCCESS = 0, RETURN_FAILURE = -1} return_sign;
typedef enum {
	SOCKET_ANONY = 0, //匿名
	SOCKET_CLOCKIN = 1, //打卡
	SOCKET_CLOCKIN_RETURN = 2, //打卡结束
	SOCKET_SERVERINFO = 3, //服务器状态
	SOCKET_TEST = 4, //测试
	SOCKET_HAVE_SEND_DATA_QUERY = 5, //查询已经传输的数据  实现断点续传
	SOCKET_SEND_DATA = 6 //传输数据
} command;


#endif /* CONSTANT_H_ */
