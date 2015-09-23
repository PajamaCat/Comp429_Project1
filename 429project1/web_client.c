//
//  main.c
//  429project1
//
//  Created by Fang on 9/21/15.
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

/* simple client, takes two parameters, the server domain name,
 and the server port number */
#define BUF_LEN 65535

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
        
        if (recv_bytes == 0)
            break;
        
        recv_msg_size += recv_bytes;
        remaining_bytes -= recv_bytes;
        
    }
    
    return recv_msg_size;
}

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
    
    /* A buffer to store received message, and a buffer to send message to server */
    char *buffer, *sendbuffer;
    
    /* number of bytes sent/received */
    ssize_t count;
    
    /* Malloc buffers */
    buffer = (char *) malloc(BUF_LEN);
    if (!buffer)
    {
        perror("failed to allocated buffer");
        abort();
    }
    
    sendbuffer = (char *) malloc(BUF_LEN);
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
    
    /* Set timestamp and send/receive messages */
        
    printf("Enter the HTTP Request: ");
    fgets(sendbuffer, BUF_LEN, stdin);
    
    printf("buffer %s \n", sendbuffer);
    
    if (strncmp(sendbuffer, "bye", 3) == 0) {
        return 0;
    }
    
    memcpy(sendbuffer + strlen(sendbuffer) - 1, "\r\n\r\n", 4);
    /* send all messages */
    count = send(sock, sendbuffer, strlen(sendbuffer), 0);
    
    /* receive all messages */
    count = recv_all(sock, BUF_LEN, buffer);
    
    printf("%s", buffer);
    

    printf("Web browser finished!\n");
    
    close(sock);
    free(buffer);
    free(sendbuffer);
    return 0;
    
}
