#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include "common.h"

int decide_drop(){
	int a = (rand()%RANGE)+1;
	if(a == DROP_NUM){
		return 1;
	}else{
		return 0;
	}
}

int main(int argc, char* argv[]){

	int drop_total = 0, total = 0;
	float lost_rate;
	/* socket with receiver */
	struct sockaddr_in recv_myaddr, recv_remaddr;
	socklen_t recv_addrlen = 	sizeof(recv_remaddr);
	int recv_sock, recv_slen =sizeof(recv_remaddr);
	char *server = "127.0.0.1";	/* change this to use a different server */
    /* receiver packet vars*/
    char* recv_mesg = malloc(PACKET_SIZE * sizeof(char));
    struct packet recv_pkt;
	/* socket for sender */
	struct sockaddr_in send_myaddr, send_remaddr;	/* remote address */
	socklen_t send_addrlen = sizeof(send_remaddr);		/* length of addresses */
	int send_sock, send_slen = sizeof(send_remaddr);			/* # bytes received */
    /* sender packet vars*/
    char* send_mesg = malloc(PACKET_SIZE * sizeof(char));
    struct packet send_pkt;

	/* build up socket which connect with receiver */
	/* create a socket */
	if ((recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("cannot create recv_sock socket\n");
		return 0;
	}
	/* bind it to all local addresses and pick any port number */
	memset((char *)&recv_myaddr, 0, sizeof(recv_myaddr));
	recv_myaddr.sin_family = AF_INET;
	recv_myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	recv_myaddr.sin_port = htons(0);

	if (bind(recv_sock, (struct sockaddr *)&recv_myaddr, sizeof(recv_myaddr)) < 0) {
		perror("bind recv_sock failed");
		return 0;
	}

	/* now define recv_remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */
	memset((char *) &recv_remaddr, 0, sizeof(recv_remaddr));
	recv_remaddr.sin_family = AF_INET;
	recv_remaddr.sin_port = htons(SERVICE_PORT);
	if (inet_aton(server, &recv_remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	printf("bind with port %d\n", SERVICE_PORT);

	/* build up socket which connect with sender */
	/* create a UDP socket */
	if ((send_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create send_sock socket\n");
		return 0;
	}
	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&send_myaddr, 0, sizeof(send_myaddr));
	send_myaddr.sin_family = AF_INET;
	send_myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	send_myaddr.sin_port = htons(AGENT_PORT);

	if (bind(send_sock, (struct sockaddr *)&send_myaddr, sizeof(send_myaddr)) < 0) {
		perror("send_sock bind failed");
		return 0;
	}
	/* now loop, receiving data and printing what we received */
	printf("Waiting on port %d\n", AGENT_PORT);

	while (1) {
		/* receive data from sender */
		if(recvfrom(send_sock, send_mesg, PACKET_SIZE, 0, (struct sockaddr *)&send_remaddr, &send_addrlen) > 0){
			total++;
			send_pkt = extract_packet(send_mesg);
			printf("Get Data #%d\n", send_pkt.num);
			printf("Fwd Data #%d\n", send_pkt.num);
			/* Start drop implementation */
			if(decide_drop() == 1){
				printf("Drop Data #%d.", send_pkt.num);
				lost_rate = (float)++drop_total/(float)total;
				printf("  loss rate: %f\n", lost_rate);
				/* receive data from sender again*/
				if(recvfrom(send_sock, send_mesg, PACKET_SIZE, 0, (struct sockaddr *)&send_remaddr, &send_addrlen) > 0){
					send_pkt = extract_packet(send_mesg);
					printf("Get Data #%d\n", send_pkt.num);
					printf("Fwd Data #%d\n", send_pkt.num);
				}else{
					perror("uh oh - something went wrong!\n");
			        exit(1);
				}
			}
			/* forward data to receiver */
			if (sendto(recv_sock, send_pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&recv_remaddr, recv_slen) < 0){
				perror("sendto");
				exit(1);
			}
			/* receive ack from receiver */
			if(recvfrom(recv_sock, recv_mesg, PACKET_SIZE, 0, (struct sockaddr *)&recv_remaddr, &recv_addrlen) > 0){
				recv_pkt = extract_packet(recv_mesg);
				printf("Get ACK #%d\n", recv_pkt.num);
				printf("Fwd ACK #%d\n", recv_pkt.num);
				/* forward ack to sender */
				if(sendto(send_sock, recv_pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&send_remaddr, send_slen) < 0){
					perror("sendto");
			        exit(1);
				}
			}else{
				perror("uh oh - something went wrong!\n");
		        exit(1);
			}
		}else{
			perror("uh oh - something went wrong!\n");
	        exit(1);
		}

	}

	return 0;
}


