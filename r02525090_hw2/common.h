#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <memory.h>

#define SERVICE_PORT 2020
#define AGENT_PORT 2016
#define BUFLEN 2048
#define TIME_THRESHOLD 1
#define PACKET_SIZE 1024
#define HEADER_SIZE (3*sizeof(int))
#define HANDSHAKE_TYPE 0
#define DATA_TYPE 1
#define ACK_TYPE 2
#define FINDATA_TYPE 3
#define FINACK_TYPE 4
#define RANGE 10
#define DROP_NUM 2

struct packet{
	int type; // 0:HANDSHAKE_TYPE, 1:DATA_TYPE, 2:ACK_TYPE, 3:FINDATA_TYPE, 4:FINACK_TYPE
	int num;
	int size;
	char data[PACKET_SIZE];
};

struct packet create_packet(int type, int num, int size, char* data){
	struct packet pkt;
	pkt.type = type;
	pkt.num = num;
	pkt.size = size;

	char* ptr = pkt.data;
    strncpy(ptr, &type, sizeof(int));
    ptr += sizeof(int);
    strncpy(ptr, &num, sizeof(int));
    ptr += sizeof(int);
    strncpy(ptr, &size, sizeof(int));
    ptr += sizeof(int);
    //copy array of real data in
    strncpy(ptr, data, size);

	return pkt;
}

/* get packet out of buffer from udp packet */
struct packet extract_packet(char* ptr){
    char* mesg = ptr;
    int type = (*(int *)mesg);
    mesg += sizeof(int);
    int num = (*(int *)mesg);
    mesg += sizeof(int);
    int size = (*(int *)mesg);
    mesg += sizeof(int);

    return create_packet(type, num, size,  mesg);
}

char* get_data(struct packet pkt){
    return pkt.data + HEADER_SIZE;
}

char * concat(char* str1, char* str2) {
      char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
      strcpy(str3, str1);
      strcat(str3, str2);

      return str3;
 }

char* random_char(){
    char* array = (char*)malloc(sizeof(char)*16);
    memset (array, 0, 16);
    time_t rawtime;
    rawtime = time(NULL);
    struct tm  *timeinfo = localtime (&rawtime);
    strftime(array, 16, "%d%m%y%H%M%S_", timeinfo);
    array[16] = '\0';
    return array;
}
