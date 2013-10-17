#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "clockin.model.h"

//打卡model
int insert_clockin(char *phoneNumber, char *phoneMac, char *datetime){
	MYSQL mysql;
	//初始化数据库
    mysql_init(&mysql);
	char *sql;
	
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof(bind));

	stmt = mysql_stmt_init(&mysql);
	sql = "insert into clockin (phone_num, phone_mac, datetime) values (?, ?, ?)";
	if(mysql_stmt_prepare(stmt, sql, strlen(sql))){
		fprintf(stderr, "mysql_stmt_prepare(), INSERT failed, %s\n", mysql_error(&mysql));
		return -1;
	}
	
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = phoneNumber;
	bind[0].buffer_length = strlen(phoneNumber);

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = phoneMac;
	bind[1].buffer_length = strlen(phoneMac);

	bind[2].buffer_type = MYSQL_TYPE_STRING;
	bind[2].buffer = datetime;
	bind[2].buffer_length = strlen(datetime);


	if(mysql_stmt_bind_param(stmt, bind)){
		fprintf(stderr, "mysql_stmt_bind_param() failed %s\n", mysql_stmt_error(stmt));
		return -1;
	}

	if(mysql_stmt_execute(stmt)){
		fprintf(stderr, "mysql_stmt_execute(), failed %s\n", mysql_stmt_error(stmt));
		return -1;
	}
	
	mysql_stmt_close(stmt);
	//mysql_close(&mysql);
	return 0;
}
