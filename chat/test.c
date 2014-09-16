#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>



char packData[200] = {0};

//打包
void pack(const char *format,...)
{
	va_list args;
	va_start(args, format);
	vsprintf(packData, format, args);
	va_end(args);
}


#define HEAD_PACKET_FORMAT "%[^,]s,%ld,%[^,]s"

typedef struct {
	char command[50];
	long contentLen;

	char extraInfo[200];
} head_packet_t;



int main()
{
	//char packData[200];

	head_packet_t head_packet;
	
	memset(&head_packet, 0, sizeof(head_packet_t));
	
	
	sprintf(head_packet.command, "%s", "send_filename");
	head_packet.contentLen = 100;
	sprintf(head_packet.extraInfo, "%s", "test.txt");
	
	//pack(HEAD_PACKET_FORMAT, head_packet.command, head_packet.contentLen, head_packet.extraInfo);
	
	sprintf(packData, "%s,%ld,%s", "send_filename", 123, "test.txt");
		

	printf("%s", packData);
	
				
		
	return 0;
}
