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
#include <semaphore.h>

#include "client_registry.h"
#include "maze.h"
#include "player.h"
#include "debug.h"
#include "server.h"

int num_clients = 0;
sem_t mutex;
sem_t empty_mutex;

struct client_registry{
    int descriptor;
    struct client_registry* prev;
  	struct client_registry* next;
};

struct client_registry* dummyHead = NULL;

CLIENT_REGISTRY *creg_init(){
	sem_init(&mutex, 0, 1);
	sem_init(&empty_mutex, 0, 1);
	dummyHead = (struct client_registry*)malloc(sizeof(struct client_registry));
	dummyHead->descriptor = -1;
	dummyHead->prev = NULL;
	dummyHead->next = NULL;
	return dummyHead;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
	sem_wait(&empty_mutex);
	while(num_clients){
		continue;
	}
	sem_post(&empty_mutex);
	return;
}

void creg_fini(CLIENT_REGISTRY *cr){
	CLIENT_REGISTRY* cur = cr;
	while(cur){
		CLIENT_REGISTRY* tmp = cur->next;
		free(cur);
		cur = tmp;
	}
	sem_destroy(&mutex);
}


void creg_register(CLIENT_REGISTRY *cr, int fd){
	sem_wait(&mutex);
	struct client_registry* tmp = cr->next;
	struct client_registry* newNode = (struct client_registry*)malloc(sizeof(struct client_registry));
	newNode->descriptor = fd;
	newNode->prev = dummyHead;
	newNode->next = tmp;
	if(tmp){
		tmp->prev = newNode;
	}
	dummyHead->next = newNode;
	num_clients++;
	sem_post(&mutex);
}

void creg_unregister(CLIENT_REGISTRY *cr, int fd){
	sem_wait(&mutex);
	struct client_registry* cur = cr;
	while(cur->descriptor != fd){
		cur = cur->next;
	}

	cur->prev->next = cur->next;
	if(cur->next){
		cur->next->prev = cur->prev;
	}
	num_clients--;
	free(cur);
	sem_post(&mutex);
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){
	sem_wait(&mutex);
	CLIENT_REGISTRY* cur = cr->next;
	while(cur){
		int fd = cur->descriptor;
		shutdown(fd, SHUT_RD);
		cur = cur->next;
	}
	sem_post(&mutex);
}