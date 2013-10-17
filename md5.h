/**
 * linux c md5
 * 依赖  openssl openssl-devel
 * yum install openssl openssl-devel
 */

#ifndef MD5_H_
#define MD5_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <openssl/hmac.h>
#include <openssl/md5.h>

void md5(const unsigned char *, char *);

void md5_file(const char *, char *);

#endif /* MD5_H_ */
