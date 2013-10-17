#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#define MEM_ALIGN(x)  (((x) + 7) & (~7))

int main()
{
	printf("%d", MEM_ALIGN(30));
	return 0;
}
