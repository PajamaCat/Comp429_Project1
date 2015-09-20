//
//  main.c
//  429project1
//
//  Created by Fang on 9/19/15.
//  Copyright © 2015 jj26&wz19. All rights reserved.
//

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

int send_all(int socket, int msg_size, char *buffer)
{
    int sent_msg_size = 0;
    ssize_t sent_bytes = 0;
    int remaining_bytes = msg_size;
    
    while (sent_msg_size < msg_size) {
        sent_bytes = send(socket, buffer + sent_msg_size, remaining_bytes, 0);
        if (sent_bytes < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("sent failure");
            abort();
        }
        sent_msg_size += sent_bytes;
        remaining_bytes -= sent_bytes;
    }
    return sent_msg_size;
}

int recv_all(int socket, int msg_size, char *buffer)
{
    int recv_msg_size = 0;
    ssize_t recv_bytes = 0;
    int remaining_bytes = msg_size;
    
    while (recv_msg_size < msg_size) {
        recv_bytes = recv(socket, buffer + recv_msg_size, remaining_bytes, 0);
        if (recv_bytes < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("recv failure");
            abort();
        }
        recv_msg_size += recv_bytes;
        remaining_bytes -= recv_bytes;
    }
    
    return recv_msg_size;
}


/* simple client, takes two parameters, the server domain name,
 and the server port number */

int main(int argc, char** argv) {
    
    /* our client socket */
    int sock;
    
    /* address structure for identifying the server */
    struct sockaddr_in sin;
    
    /* convert server domain name to IP address */
    struct hostent *host = gethostbyname(argv[1]);
    unsigned int server_addr = *(unsigned int *) host->h_addr_list[0];
    
    /* server port number */
    unsigned short server_port = atoi (argv[2]);
    
    /* The size in bytes of each message to send (10 <= size <= 65,535) */
    unsigned short msg_size = atoi(argv[3]);
    if (msg_size < 0 || msg_size > 65535) {
        perror("Input message size should between 10 and 65535.");
        abort();
    }
    
    /* The number of message exchanges to perform (1 <= count <= 10,000)*/
    int msg_count = atoi(argv[4]);
    
    /* Timeval struct for gettimeofday() */
    struct timeval start, end;
    long time_diff;
    
    /* A buffer to store received message, and a buffer to send message to server */
    char *buffer, *sendbuffer;
    
    /* Malloc buffers */
    buffer = (char *) malloc(msg_size);
    if (!buffer)
    {
        perror("failed to allocated buffer");
        abort();
    }
    
    sendbuffer = (char *) malloc(msg_size);
    if (!sendbuffer)
    {
        perror("failed to allocated sendbuffer");
        abort();
    }
    
    
    /* create a socket */
    if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror ("opening TCP socket");
        abort ();
    }
    
    /* fill in the server's address */
    memset (&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(server_port);
    
    /* connect to the server */
    if (connect(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("connect to server failed");
        abort();
    }
	
    /* setup sendbuffer with an ending zero */
    
    memcpy(sendbuffer, &msg_size, 2);
    unsigned int start_sec;
    unsigned int start_usec;
    unsigned int end_sec;
    unsigned int end_usec;
    unsigned int diff;	// difference between start_usec and end_usec
    
    int i;
    for (i = 10; i < msg_size - 1; i++) {
        sendbuffer[i] = 'x';
    }
    sendbuffer[msg_size - 1] = 0;
    
    /* Set timestamp and send/receive messages */
    while (msg_count > 0) {
        gettimeofday(&start, NULL);
		
        start_sec = (unsigned int)start.tv_sec;
        start_usec = (unsigned int)start.tv_usec;
        
        memcpy(sendbuffer+2, &start_sec, 4);
        memcpy(sendbuffer+6, &start_usec, 4);
        
        /* send all messages */
        send_all(sock, msg_size, sendbuffer);

		/* receive all messages */
        recv_all(sock, msg_size, buffer);
        
        /* copy back start timestamp */
        memcpy(&start_sec, buffer + 2, 4);
        memcpy(&start_usec, buffer + 6, 4);
        
        gettimeofday(&end, NULL);
        
        end_sec = (unsigned int)end.tv_sec;
        end_usec = (unsigned int)end.tv_usec;
        
        diff = end_usec - start_usec;
        
        time_diff = (end_sec - start_sec) * 1000000 + (diff > 0 ? diff : diff + pow(2, 32));
        printf("Latency for %d bytes message is %lu.\n", msg_size, time_diff);
        
        msg_count--;
    }
    
    printf("Ping-Pong finished!\n");
    
    close(sock);
    free(buffer);
    free(sendbuffer);
    return 0;

}
