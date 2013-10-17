export C_INCLUDE_PATH = $C_INCLUDE_PATH:/usr/local/mysql/include/mysql/
export LIBRARY_PATH = $LIBRARY_PATH:/usr/local/mysql/lib/mysql/
CC = gcc
all:server.c cJSON.c clockin.model.c md5.c upload.c
	$(CC) -g -Wall server.c upload.c util.c cJSON.c clockin.model.c md5.c -lpthread -lssl -lm -lmysqlclient -DDEBUG -o server
clean:
	rm -rf server
	