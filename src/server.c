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

int debug_show_maze = 1;
CLIENT_REGISTRY *client_registry;

void sigusrHandler(int sig){
    printf("Laser Hit\n");
}

void *mzw_client_service(void *vargp)
{
    struct sigaction handler;
    handler.sa_handler = sigusrHandler;
    sigemptyset (&handler.sa_mask);
    handler.sa_flags = 0;
    sigaction(SIGUSR1, &handler, NULL);
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    creg_register(client_registry, connfd);
    PLAYER* curplayer = NULL;
    show_maze();
    while(1){
        if(curplayer){
            player_check_for_laser_hit(curplayer);
        }
    	MZW_PACKET *packet = malloc(sizeof(MZW_PACKET));
        char *data = NULL;
    	int val = proto_recv_packet(connfd, packet, (void **)&data);
        if(val > 0){
            if(errno!=EINTR){
                free(packet);
                free(data);
                break;
            }
            continue;
        }
    	int type = packet->type;

    	if(type == 1){
            if(curplayer){
                MZW_PACKET *pkt = malloc(sizeof(MZW_PACKET));
                pkt->type = 8;
                proto_send_packet(connfd, pkt, NULL);
            }
            else{
                curplayer = player_login(connfd, packet->param1, data);
                MZW_PACKET *pkt = malloc(sizeof(MZW_PACKET));
                if(!curplayer){
                    pkt->type = 8;
                    proto_send_packet(connfd, pkt, NULL);
                }
                else{
                    pkt->type = 7;
                    proto_send_packet(connfd, pkt, NULL);
                    player_reset(curplayer);
                }
            }
    	}
        else if(type == 2){
            player_move(curplayer, packet->param1);
        }
        else if(type == 3){
            player_rotate(curplayer, packet->param1);
        }
        else if(type == 4){
            player_fire_laser(curplayer);
        }
        else if(type == 5){
            player_invalidate_view(curplayer);
            player_update_view(curplayer);
        }
        else if(type == 6){
            player_send_chat(curplayer, data, sizeof(data));
        }
    	free(packet);
    	free(data);
        if(debug_show_maze) {
            show_maze();
        }
    }
    if(curplayer){
        player_logout(curplayer);
    }
    close(connfd);
    creg_unregister(client_registry, connfd);
    return NULL;
}