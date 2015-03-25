CC=gcc
CFLAGS = -g 
# uncomment this for SunOS
# LIBS = -lsocket -lnsl

all: send recv agent

send: r02525090_hw2_send.o 
	$(CC) -o send r02525090_hw2_send.o $(LIBS)
recv: r02525090_hw2_recv.o 
	$(CC) -o recv r02525090_hw2_recv.o $(LIBS)
agent:r02525090_hw2_agent.o
	$(CC) -o agent r02525090_hw2_agent.o $(LIBS)

r02525090_hw2_send.o: r02525090_hw2_send.c common.h
r02525090_hw2_recv.o: r02525090_hw2_recv.c common.h
r02525090_hw2_agent.o:r02525090_hw2_agent.c common.h

clean:
	rm -f send recv agent send.o recv.o agent.o 
