//
//  ping_pong_server.c
//  429project1
//
//  Created by Fang on 9/19/15.
//  Copyright © 2015 jj26&wz19. All rights reserved.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUF_LEN 65535
#define OK	"200 OK\r\n"
#define CONTENT_TYPE "Content-Type: text/html\r\n"
#define NOT_IMPLEMENTED "501 Not Implemented\r\n"
#define NOT_FOUND "404 Not Found\r\n"
#define BAD_REQUEST "400 Bad Request\r\n"
/**************************************************/
/* a few simple linked list functions             */
/**************************************************/


/* A linked list node data structure to maintain application
 information related to a connected socket */
struct node {
    int socket;
    struct sockaddr_in client_addr;
    int pending_data; /* flag to indicate whether there is more data to send */
    /* you will need to introduce some variables here to record
     all the information regarding this socket.
     e.g. what data needs to be sent next */
    struct node *next;
    char buffer[BUF_LEN];
    int pointer;
    FILE *fp;
    long remaining_file_size;
};

/* remove the data structure associated with a connected socket
 used when tearing down the connection */
void dump(struct node *head, int socket) {
    struct node *current, *temp;
    
    current = head;
    
    while (current->next) {
        if (current->next->socket == socket) {
            /* remove */
            temp = current->next;
            current->next = temp->next;
            free(temp); /* don't forget to free memory */
            return;
        } else {
            current = current->next;
        }
    }
}

/* create the data structure associated with a connected socket */
void add(struct node *head, int socket, struct sockaddr_in addr) {
    struct node *new_node;
    
    new_node = (struct node *)malloc(sizeof(struct node));
    new_node->socket = socket;
    new_node->client_addr = addr;
    new_node->pending_data = 0;
    new_node->next = head->next;
    new_node->pointer = 0;
    new_node->remaining_file_size = 0;
    memset(new_node->buffer, 0, BUF_LEN);
    head->next = new_node;
}

void read_file(struct node *current, long available_space) {
    printf("aaaaaaaa\n");
    if (current->remaining_file_size > available_space) {
        printf("b\n");
        fread(current->buffer+current->pointer,
              available_space, 1, current->fp);
        printf("bb %lu\n", available_space);

        fseek(current->fp, available_space, SEEK_CUR);
        current->remaining_file_size -= available_space;
    } else {
        fread(current->buffer+current->pointer,
              current->remaining_file_size, 1, current->fp);
        current->remaining_file_size = 0;
        fclose(current->fp);
    }
}

/*****************************************/
/* main program                          */
/*****************************************/

/* simple server, takes one parameter, the server port number */
int main(int argc, char **argv) {
    
    /* socket and option variables */
    int sock, new_sock, max;
    int optval = 1;
    
    /* server socket address variables */
    struct sockaddr_in sin, addr;
    unsigned short server_port = atoi(argv[1]);
    
    /* check if we are in www mode */
    int www_mode = 0;
    if (argv[2] != NULL && strncmp(argv[2], "www", 3) == 0) {
        www_mode = 1;
    }
    
    /* socket address variables for a connected client */
    socklen_t addr_len = sizeof(struct sockaddr_in);
    
    /* maximum number of pending connection requests */
    int BACKLOG = 5;
    
    /* variables for select */
    fd_set read_set, write_set;
    struct timeval time_out;
    int select_retval;
    
    /* number of bytes sent/received */
    ssize_t count;
    
    /* linked list for keeping track of connected sockets */
    struct node head;
    struct node *current, *next;
    
    /* size of msg received */
    unsigned short msg_size;

    /* initialize dummy head node of linked list */
    head.socket = -1;
    head.next = 0;
    
    /* create a server socket to listen for TCP connection requests */
    if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror ("opening TCP socket");
        abort ();
    }
    
    /* set option so we can reuse the port number quickly after a restart */
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval)) <0)
    {
        perror ("setting TCP socket option");
        abort ();
    }
    
    /* fill in the address of the server socket */
    memset (&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (server_port);
    
    /* bind server socket to the address */
    if (bind(sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("binding socket to address");
        abort();
    }
    
    /* put the server socket in listen mode */
    if (listen (sock, BACKLOG) < 0)
    {
        perror ("listen on socket failed");
        abort();
    }
    
    /* now we keep waiting for incoming connections,
     check for incoming data to receive,
     check for ready socket to send more data */
    while (1)
    {
        
        /* set up the file descriptor bit map that select should be watching */
        FD_ZERO (&read_set); /* clear everything */
        FD_ZERO (&write_set); /* clear everything */
        
        FD_SET (sock, &read_set); /* put the listening socket in */
        max = sock; /* initialize max */
        
        /* put connected sockets into the read and write sets to monitor them */
        for (current = head.next; current; current = current->next) {
            FD_SET(current->socket, &read_set);
            
            if (current->pending_data) {
                /* there is data pending to be sent, monitor the socket
                 in the write set so we know when it is ready to take more
                 data */
                FD_SET(current->socket, &write_set);
            }
            
            if (current->socket > max) {
                /* update max if necessary */
                max = current->socket;
            }
        }
        
        time_out.tv_usec = 100000; /* 1-tenth of a second timeout */
        time_out.tv_sec = 0;
        
        /* invoke select, make sure to pass max+1 !!! */
        select_retval = select(max+1, &read_set, &write_set, NULL, &time_out);
        if (select_retval < 0)
        {
            perror ("select failed");
            abort ();
        }
        
        if (select_retval == 0)
        {
            /* no descriptor ready, timeout happened */
            continue;
        }
        
        if (select_retval > 0) /* at least one file descriptor is ready */
        {
            if (FD_ISSET(sock, &read_set)) /* check the server socket */
            {
                /* there is an incoming connection, try to accept it */
                new_sock = accept (sock, (struct sockaddr *) &addr, &addr_len);
                
                if (new_sock < 0)
                {
                    perror ("error accepting connection");
                    abort ();
                }
                
                /* make the socket non-blocking so send and recv will
                 return immediately if the socket is not ready.
                 this is important to ensure the server does not get
                 stuck when trying to send data to a socket that
                 has too much data to send already.
                 */
                if (fcntl (new_sock, F_SETFL, O_NONBLOCK) < 0)
                {
                    perror ("making socket non-blocking");
                    abort ();
                }
                
                /* the connection is made, everything is ready */
                /* let's see who's connecting to us */
                printf("Accepted connection. Client IP address is: %s\n",
                       inet_ntoa(addr.sin_addr));
                
                /* remember this client connection in our linked list */
                add(&head, new_sock, addr);

            }
            
            /* check other connected sockets, see if there is
             anything to read or some socket is ready to send
             more pending data */
            for (current = head.next; current; current = next) {
                next = current->next;
                
                /* see if we can now do some previously unsuccessful writes */
                if (FD_ISSET(current->socket, &write_set)) {
                    /* the socket is now ready to take more data */
                    /* the socket data structure should have information
                     describing what data is supposed to be sent next.
                     but here for simplicity, let's say we are just
                     sending whatever is in the buffer buf
                     */
                    if (www_mode) {
                        printf("current pointer is at %d\n", current->pointer);

                        count = send(current->socket,
                                     current->buffer+current->pointer,
                                     strlen(current->buffer)-current->pointer, MSG_DONTWAIT);
                        if (count < 0) {
                            if (errno == EAGAIN) {
                                // No-op
                            } else {
                                printf("Something is wrong while sending to Client IP address: %s\n", inet_ntoa(current->client_addr.sin_addr));
                                close(current->socket);
                                dump(&head, current->socket);
                            }
                            continue;
                        }

                        current->pointer += count;
                        if(current->buffer + current->pointer == 0
                           || current->pointer == strlen(current->buffer)) {	// end of file or end of buffer
                            current->pointer = 0;
                            current-> pending_data = 0;
                        }
                        
                        printf("REMAINING FILE_SIZE %lu\n", current->remaining_file_size);
                        /* if there's no pending file to read, close connection. otherwise do nothing */
                        if (current->remaining_file_size == 0L) {
                            printf("Close connection with client. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
                            close(current->socket);
                            dump(&head, current->socket);
                        }
                        
                    } else {
                        memcpy(&msg_size, current->buffer, 2);
                        count = send(current->socket, current->buffer+current->pointer, msg_size-current->pointer, MSG_DONTWAIT);
                        
                        if (count < 0) {
                            if (errno == EAGAIN) {
                                // No-op
                            } else {
                                printf("Something is wrong while sending to Client IP address: %s\n", inet_ntoa(current->client_addr.sin_addr));
                                close(current->socket);
                                dump(&head, current->socket);
                            }
                            continue;
                        }
                        
                        current->pointer += count;
                        if(current->pointer == msg_size) {
                            current->pointer = 0;
                            current-> pending_data = 0;
                        }
                    }
                }
                
                if (FD_ISSET(current->socket, &read_set)) {
                    
                    if (www_mode) {
                        
                        /* if there's file remaining to read, read file, otherwise receive message */
                        if (current->remaining_file_size > 0) {
                            read_file(current, BUF_LEN);
                        } else {
                            /* we have data from a client */
                            count = recv(current->socket, current->buffer+current->pointer, BUF_LEN, 0);
                            
                            if (count <= 0) {
                                if (errno == EAGAIN) {
                                    // No-op
                                } else {
                                    /* something is wrong */
                                    if (count == 0) {
                                        printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
                                    } else {
                                        perror("error receiving from a client");
                                    }
                                    
                                    /* connection is closed, clean up */
                                    close(current->socket);
                                    dump(&head, current->socket);
                                }
                                continue;
                            }
                            
                            printf("CURRENT BUFFER IS %s\n", current->buffer);
                            if (strncmp(current->buffer + count - 4 , "\r\n\r\n", 4) != 0) {
                                continue;
                            }
                            
                            if (strncmp(current->buffer, "GET", 3) == 0) {
                                printf("PASSED GET\n");
                                char *separator = strpbrk(current->buffer + 3, "/");
                                char *path_begin = separator;
                                while (*separator != ' ') {
                                    separator++;
                                }
                                
                                /* get file path */
                                char filepath[separator-path_begin+1];
                                memcpy(filepath, path_begin, separator-path_begin);
                                filepath[separator-path_begin] = 0;
                                
                                char *http_version = strstr(current->buffer, "HTTP");
                                
                                if (http_version != NULL && strstr(filepath, "../") == NULL) {
                                    char *http_begin = http_version;
                                    while (*http_version != '\r') {
                                        http_version++;
                                    }
                                    
                                    /* add http version */
                                    memcpy(current->buffer, http_begin, http_version-http_begin);
                                    current->pointer += http_version-http_begin;
                                    memcpy(current->buffer + current->pointer, " ", 1);
                                    current->pointer++;
                                    
                                    printf("%d\n", access(filepath, F_OK));
                                    /* check if the file exists */
                                    if (access(filepath, F_OK) != -1) {
                                        printf("PASSED filecheck\n");
                                        if (strstr(filepath, ".html") != NULL || strstr(filepath, ".txt") != NULL) {
                                            memcpy(current->buffer+current->pointer, OK, sizeof(OK)-1);
                                            current->pointer += sizeof(OK)-1;
                                            printf("size of OK %lu\n", sizeof(OK));
                                            memcpy(current->buffer+current->pointer,
                                                   CONTENT_TYPE,
                                                   sizeof(CONTENT_TYPE)-1);
                                            current->pointer += sizeof(CONTENT_TYPE)-1;
                                            
                                            /* read file */
                                            current->fp = fopen(filepath, "r");
                                            struct stat st;
                                            stat(filepath, &st);
                                            
                                            current->remaining_file_size = st.st_size;
                                            long remaining_buffer_size = BUF_LEN - strlen(current->buffer);
                                            
                                            printf("remaining %lu?\n", remaining_buffer_size);
                                            printf("filesize %lu\n", current->remaining_file_size);
                                            read_file(current, remaining_buffer_size);
                                            printf("After read file %lu\n", strlen(current->buffer));
                                            
                                        } else {
                                            /* unsupported file type */
                                            memset(current->buffer+current->pointer, 0, sizeof(NOT_IMPLEMENTED));
                                            memcpy(current->buffer+current->pointer,
                                                   NOT_IMPLEMENTED,
                                                   sizeof(NOT_IMPLEMENTED)-1);
                                            current->pointer += sizeof(NOT_IMPLEMENTED)-1;
                                            current->buffer[strlen(current->buffer)] = 0;
                                        }
                                        
                                    } else {	// file does not exist
                                        memset(current->buffer+current->pointer, 0, sizeof(NOT_FOUND));
                                        memcpy(current->buffer+current->pointer,
                                               NOT_FOUND,
                                               sizeof(NOT_FOUND)-1);
                                        current->pointer += sizeof(NOT_FOUND)-1;
                                        current->buffer[strlen(current->buffer)] = 0;
                                    }
                                } else {	// 	request does not contain http version
                                    memset(current->buffer+current->pointer, 0, sizeof(BAD_REQUEST));
                                    memcpy(current->buffer+current->pointer,
                                           BAD_REQUEST,
                                           sizeof(BAD_REQUEST)-1);
                                    
                                    current->pointer += sizeof(BAD_REQUEST)-1;
                                    current->buffer[strlen(current->buffer)] = 0;
                                }
                                
                            } else {	// 	request is not get
                                memset(current->buffer+current->pointer, 0, sizeof(BAD_REQUEST));
                                memcpy(current->buffer+current->pointer,
                                       BAD_REQUEST,
                                       sizeof(BAD_REQUEST)-1);
                                current->pointer += sizeof(BAD_REQUEST)-1;
                                current->buffer[strlen(current->buffer)] = 0;
                            }
                        }
                        current->pending_data = 1;
                        current->pointer = 0;
                        
                    } else { // non WWW MODE
                        /* we have data from a client */
                        count = recv(current->socket, current->buffer+current->pointer, BUF_LEN, 0);
                        
                        if (count <= 0) {
                            if (errno == EAGAIN) {
                                // No-op
                            } else {
                                /* something is wrong */
                                if (count == 0) {
                                    printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
                                } else {
                                    perror("error receiving from a client");
                                }
                                
                                /* connection is closed, clean up */
                                close(current->socket);
                                dump(&head, current->socket);
                            }
                            continue;
                        }
                        memcpy(&msg_size, current->buffer, 2);
                        current->pointer += count;
                        if(current->pointer == msg_size) {
                            current->pointer = 0;
                            current-> pending_data = 1;
                        }
                    }
                }
            }
        }
    }
}
