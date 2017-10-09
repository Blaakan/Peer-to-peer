#ifndef PEER_H
#define PEER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ADDRESS "127.0.0.1"

#define BUF_SIZE 512

typedef struct node
{
	int ID;
	int ext;
	int ring_prec;
	int ring_next;
	struct sockaddr_in selfaddr;
	struct sockaddr_in nextaddr;
}node;

int checkcmd(char * cmd, char * message);

char ** split(char * Str, char delim);

#endif
