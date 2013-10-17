export C_INCLUDE_PATH = $C_INCLUDE_PATH:/mnt/hgfs/bigfileupload/c/:/usr/local/mysql/include/mysql/
export LIBRARY_PATH = $LIBRARY_PATH:/usr/local/mysql/lib/mysql/
CC = gcc
all:client.c ../util.c ../md5.c ../upload.c ../cJSON.c ../clockin.model.c
	$(CC) -g -Wall client.c ../util.c ../md5.c ../upload.c ../cJSON.c ../clockin.model.c -DDEBUG  -lssl -lm   -lpthread  -lmysqlclient   -o client
clean:
	rm -rf client