#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"

int check_timeout(int sock){
	int result;
	fd_set fds, read_fds;
	struct timeval timeout = {TIME_THRESHOLD, 0};

	FD_ZERO(&read_fds);
	FD_SET(sock, &read_fds);
	if(select(sock+1, &read_fds, NULL, NULL, &timeout) <0){
		perror("[ERROR]timeout error \n");
		exit(1);
	}

	//waiting for result
	if(FD_ISSET(sock, &read_fds)){
		result = 0;
	}else{
		result = 1;  //time out!
	}

	return result;
}

int main(int argc, char* argv[]){

	struct sockaddr_in myaddr, remaddr;
	int sock, slen=sizeof(remaddr);
	int recvlen;		/* bytes in acknowledgement message */
	char buf[BUFLEN];	/* message buffer */
	char *server = "127.0.0.1";	/* change this to use a different server */
	int send_amount, timeout_result;
    /* these will hold file  */
    int filesize;
	int file;
    char* filebuffer;
	char* filename;
    /* packet vars*/
    char mesg[PACKET_SIZE];
    int seq = 0;
    struct packet pkt;

    if (argc < 2) {
      printf("Usage: send <filename>\n");
      return 0;
    }
    filename = argv[1];

	/* create a socket */
	if ((sock=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("cannot create socket\n");
		return 0;
	}
	/* bind it to all local addresses and pick any port number */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */
	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(AGENT_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	/* now let's send the messages
	 * open file ->read ->send
	 * */
	file = open(filename, O_RDONLY);
	if(file < 0){
		perror("Can't find file for reading!");
		exit(1);
	}
    struct stat st;
    stat(filename, &st);
    filesize = st.st_size;
    filebuffer = (char *)malloc(filesize * sizeof(char));
    read(file, filebuffer, filesize);

    while(1){
        if (filesize > PACKET_SIZE - HEADER_SIZE){
        	/* wrap packet */
        	pkt = create_packet(DATA_TYPE, ++seq, PACKET_SIZE - HEADER_SIZE, filebuffer);
        	/* send packet */
        	send_amount = (int)sendto(sock, pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, slen);
        }else{
        	printf("Last packet!");
        	/* wrap packet */
        	pkt = create_packet(FINDATA_TYPE, ++seq, filesize, filebuffer);
        	/* send packet */
        	send_amount = (int)sendto(sock, pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, slen);
        }

    	if(send_amount == -1){
    		perror("sendto");
    		exit(1);
    	}
    	printf("Send Data #%d\n", seq);
    	/*check time out*/
    	timeout_result = check_timeout(sock);
    	/* If receiving no corresponding acknowledgement during the time interval,
    	 * sender will retransmit the packet until receives the acknowledgement.
    	 * */
    	while(timeout_result > 0){
    		printf("Time out! Data #%d\n", seq);
    		send_amount = (int)sendto(sock, pkt.data, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, slen);
    		if(send_amount == -1){
    			perror("sendto");
    			exit(1);
    		}
    		printf("Resend Data #%d\n", seq);
    		timeout_result = check_timeout(sock);
    	}
    	/*receive ack from agent*/
    	bzero(mesg, sizeof(mesg));
    	recvlen = recvfrom(sock, mesg, PACKET_SIZE, 0, (struct sockaddr *)&remaddr, &slen);
    	if(recvlen > 0){
    		struct packet ack = extract_packet(mesg);
    		if(ack.type == FINACK_TYPE){
    			printf("Recv FIN ACK #%d\n", ack.num);
    			break;
    		}else{
    			printf("Recv Ack #%d\n", ack.num);
    			filesize -= (PACKET_SIZE - HEADER_SIZE);
    			filebuffer += (PACKET_SIZE - HEADER_SIZE);
    		}

    	}
    }

	close(sock);
	return 0;
}
