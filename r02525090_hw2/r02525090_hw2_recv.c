#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "common.h"

#define BUFSIZE 2048

int main(int argc, char* argv[]){

	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int sock;				/* our socket */
	int msgcnt = 0;			/* count # of messages we received */
	unsigned char buf[BUFSIZE];	/* receive buffer */
	/* file vars*/
	char* filename;
	int file;
    /* packet vars*/
    char* mesg = malloc(PACKET_SIZE * sizeof(char));
    int seq = 0;
    struct packet pkt;

    if (argc < 2) {
      printf("Usage: recv <filename>\n");
      exit(1);
    }
    filename = concat(random_char(),argv[1]);

	/* create a UDP socket */
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}
	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(SERVICE_PORT);

	if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	printf("Waiting on port %d\n", SERVICE_PORT);
	printf("New file name:%s\n", filename);
    if ((file = open(filename, O_WRONLY|O_CREAT|O_EXCL)) < 0){
        perror("Error file opening");
        exit(1);
    }

	while (1) {
		/*receive data from agent*/
		recvlen = recvfrom(sock, mesg, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			pkt = extract_packet(mesg);
            if (write(file, get_data(pkt), pkt.size) < 0){
                perror("Write failed!");
                exit(1);
            }
            printf("Recv Data #%d\n", pkt.num);
    		/*If it's the last packet, reply fin ack. Otherwise, just reply ack.*/
			if(pkt.type == DATA_TYPE){
				struct packet ack_pkt = create_packet(ACK_TYPE, pkt.num, 0, NULL);
				printf("Send Ack #%d\n", pkt.num);
				if (sendto(sock, ack_pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, addrlen) < 0){
					perror("sendto");
					exit(1);
				}
			}else if(pkt.type == FINDATA_TYPE){
				struct packet ack_pkt = create_packet(FINACK_TYPE, pkt.num, 0, 0);
				printf("Send FIN ACK #%d\n", pkt.num);
				if (sendto(sock, ack_pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, addrlen) < 0){
					perror("sendto");
				}else{
					close(file);
				}
			}
		}else{
			perror("uh oh - something went wrong!\n");
	        exit(1);
		}
	}
	/* never exits */
}
