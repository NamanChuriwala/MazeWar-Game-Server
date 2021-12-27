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

#include "client_registry.h"
#include "maze.h"
#include "player.h"
#include "debug.h"
#include "server.h"

typedef struct sockaddr SA;

static void terminate(int status);

static char *default_maze[] = {
  "******************************",
  "***** %%%%%%%%% &&&&&&&&&&& **",
  "***** %%%%%%%%%        $$$$  *",
  "*           $$$$$$ $$$$$$$$$ *",
  "*##########                  *",
  "*########## @@@@@@@@@@@@@@@@@*",
  "*           @@@@@@@@@@@@@@@@@*",
  "******************************",
  NULL
};


int get_file_cols(char* filename){
    char buffer[1500];
    FILE* file = fopen(filename, "r");
    fgets(buffer, sizeof(buffer), file);
    fclose(file);
    return strlen(buffer)-1;
}

int get_file_rows(char* filename){
    char buffer[1500];
    FILE* file = fopen(filename, "r");
    int cnt = 0;
    while(fgets(buffer, sizeof(buffer), file)){
        cnt++;
    }
    fclose(file);
    return cnt;
}

char** template_maze;

CLIENT_REGISTRY *client_registry;
void sigHandler(int);
int serverSocket;
int PORT;
char *filename;
int flag = 0;

int main(int argc, char* argv[]){
    filename = "none";
    struct sigaction handler;
    handler.sa_handler = sigHandler;
    sigemptyset (&handler.sa_mask);
    handler.sa_flags = 0;
    sigaction(SIGINT, &handler, NULL);
    sigaction(SIGHUP, &handler, NULL);
    int c;

    while(1){
        static struct option long_options[] =
        {
          {"port-number", optional_argument, NULL, 'p'},
          {"template-file", optional_argument, NULL, 't'},
          {0, 0, 0, 0}
        };
        c = getopt_long (argc, argv, "p:t:",
                       long_options, NULL);

        if (c == -1){
          break;
        }

        switch(c){
          case 0:
            break;
          case 'p':
            PORT = atoi(optarg);
            break;
          case 't':
            filename = optarg;
            break;
          default:
            fprintf(stderr, "Invalid Args\n");
            exit(1);
        }

  }

    if(strcmp(filename,"none") != 0){
        flag = 1;
        char buffer[1500];
        int cols = get_file_cols(filename);
        int rows = get_file_rows(filename);
        template_maze = malloc(sizeof(char*)*(rows+1));
        FILE* file = fopen(filename, "r");
        int i = 0;
        while(fgets(buffer, sizeof(buffer), file)){
            *(template_maze+i) = malloc(sizeof(char)*cols);
            for(int j=0; j<cols; j++){
                *(*(template_maze+i)+j) = buffer[j];
            }
            i++;
        }
        *(template_maze+i) = NULL;
        fclose(file);
    }

    client_registry = creg_init();
    if(flag == 1){
        maze_init(template_maze);
        int rc = get_file_rows(filename) + 1;
        for(int i = 0; i < rc; i++)
            free(template_maze[i]);
        free(template_maze);
    }
    else{
        maze_init(default_maze);
    }
    player_init();
    debug_show_maze = 1;

    socklen_t clientlen;
    int *connfdp;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage clientaddr;

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    bind(serverSocket,
         (struct sockaddr*)&serverAddr,
         sizeof(serverAddr));

    listen(serverSocket, 50);

    pthread_t tid;

    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        *connfdp = accept(serverSocket, (struct sockaddr *)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, mzw_client_service, connfdp);
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the MazeWar server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    player_fini();
    maze_fini();

    debug("MazeWar server terminating");
    exit(status);
}

void sigHandler(int sig){
    terminate(0);
}

void sigHupHandler(int sig){
    terminate(0);
}