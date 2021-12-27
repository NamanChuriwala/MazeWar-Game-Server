#define _GNU_SOURCE
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include<stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include<errno.h>

#include "client_registry.h"
#include "maze.h"
#include "player.h"
#include "debug.h"
#include "server.h"

int proto_send_packet(int fd, MZW_PACKET *pkt, void *data){
	ssize_t size = pkt->size;
	pkt->size = htons(pkt->size);
	pkt->timestamp_sec = htonl(pkt->timestamp_sec);
	pkt->timestamp_nsec = htonl(pkt->timestamp_nsec);
	int rv = write(fd, pkt, sizeof(MZW_PACKET));
	if(rv <= 0){
		return 1;
	}
	if(data != NULL){
		int rv1 = write(fd, data, size);
		if(rv1 <= 0){
			return 1;
		}
	}
	return 0;
}


int proto_recv_packet(int connfd, MZW_PACKET *packet, void **data){
    int rv1 = read(connfd, packet, sizeof(MZW_PACKET));
    if(rv1 == 0){
    	errno = 12;
    	return 1;
    }
    else if(rv1 < 0){
    	return 1;
    }
    packet->size = ntohs(packet->size);
    packet->timestamp_sec = ntohl(packet->timestamp_sec);
	packet->timestamp_nsec = ntohl(packet->timestamp_nsec);
	*data = malloc(packet->size);

    if(packet->size > 0){
        int rv2 = read(connfd, *data, packet->size);
        if(rv2 == 0){
     		errno = 12;
    		return 1;
    	}
    	else if(rv2 < 0){
    		return 1;
    	}
    }

	return 0;
}
