
# This is a sample Makefile which compiles source files named:
# - tcpechotimeserv.c
# - tcpechotimecliv.c
# - time_cli.c
# - echo_cli.c
# and creating executables: "server", "client", "time_cli"
# and "echo_cli", respectively.
#
# It uses various standard libraries, and the copy of Stevens'
# library "libunp.a" in ~cse533/Stevens/unpv13e_solaris2.10 .
#
# It also picks up the thread-safe version of "readline.c"
# from Stevens' directory "threads" and uses it when building
# the executable "server".
#
# It is set up, for illustrative purposes, to enable you to use
# the Stevens code in the ~cse533/Stevens/unpv13e_solaris2.10/lib
# subdirectory (where, for example, the file "unp.h" is located)
# without your needing to maintain your own, local copies of that
# code, and without your needing to include such code in the
# submissions of your assignments.
#
# Modify it as needed, and include it with your submission.

CC = gcc

LIBS = -lresolv -lsocket -lnsl -lpthread\
       /home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a\
      
FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

OBJECT = circular_buffer.o Packet.o Linkedlist.o SockTb.o get_myinfo.o rtt.o

all: client server

client: client.o ${OBJECT}
	${CC} -o client client.o ${OBJECT} ${LIBS}

client.o: client.c
	${CC} ${CFLAGS} -c client.c

server: server.o ${OBJECT}
	${CC} -o server server.o ${OBJECT} ${LIBS}

server.o: server.c 
	${CC} ${CFLAGS} -c server.c

get_myinfo.o: /home/stufs1/fayang/hw2/get_myinfo.c
	${CC} ${CFLAGS} -c /home/stufs1/fayang/hw2/get_myinfo.c

circular_buffer.o: /home/stufs1/fayang/hw2/circular_buffer.c
	${CC} ${CFLAGS} -c /home/stufs1/fayang/hw2/circular_buffer.c

Packet.o: /home/stufs1/fayang/hw2/Packet.c
	${CC} ${CFLAGS} -c /home/stufs1/fayang/hw2/Packet.c

Linkedlist.o: /home/stufs1/fayang/hw2/Linkedlist.c
	${CC} ${CFLAGS} -c /home/stufs1/fayang/hw2/Linkedlist.c

SockTb.o: /home/stufs1/fayang/hw2/SockTb.c
	${CC} ${CFLAGS} -c /home/stufs1/fayang/hw2/SockTb.c

rtt.o: /home/stufs1/fayang/hw2/rtt.c
	${CC} ${CFLAGS} -c /home/stufs1/fayang/hw2/rtt.c

clean:
	rm  client client.o server server.o 
