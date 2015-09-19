CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= 
DEFS 	 	=

all:	ping_pong_client ping_pong_server


ping_pong_server: ping_pong_server.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ping_pong_server ping_pong_server.c

ping_pong_client: ping_pong_client.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o ping_pong_client ping_pong_client.c

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f ping_pong_client
	rm -f ping_pong_server
