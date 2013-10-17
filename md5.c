#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"

/**
 * @param const unsigned char * toBeEncryptStr  待加密的字符串常量
 * @param char * encryptStr  已经加密的字符串  32位
 *
 * 使用范例
 * char encryptStr[33] = {'\0'};
 * md5("lijianwei", encryptStr);
 */
void md5(const  unsigned char * toBeEncryptStr, char *encryptStr)
{
	unsigned char md[17] = {0};
	int i = 0;
	MD5(toBeEncryptStr, strlen((char *)toBeEncryptStr), md);
	char tmp[3] = {'\0'};

	for (i = 0; i < MD5_DIGEST_LENGTH; i++){
		sprintf(tmp, "%02x", md[i]);
		strcat(encryptStr, tmp);
	}
}

void md5_file(const char * filepath, char *encryptStr)
{
        int n;
        MD5_CTX c;
        char buf[512];
        ssize_t bytes;

        MD5_Init(&c);
        int fd = open(filepath, O_RDONLY);
        bytes=read(fd, buf, 512);
        while(bytes > 0)
        {
                MD5_Update(&c, buf, bytes);
                bytes=read(fd, buf, 512);
        }
        unsigned char out[17] = {'\0'};
        char tmp[3] = {'\0'};

        MD5_Final(out, &c);

        for(n=0; n<MD5_DIGEST_LENGTH; n++) {
        	sprintf(tmp, "%02x", out[n]);
        	strcat(encryptStr, tmp);
        }

#ifdef DEBUG
        printf("file %s md5 %s\n", filepath, encryptStr);
#endif

        close(fd);
}

