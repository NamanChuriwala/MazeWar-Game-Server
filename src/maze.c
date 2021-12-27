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
#include <time.h>

#include "client_registry.h"
#include "maze.h"
#include "player.h"
#include "debug.h"
#include "server.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
sem_t maze_mutex;
int total_rows = 0;
int total_cols = 0;

int getRows(char** newmaze1){
    int rows = 0;
    char **cur = newmaze1;
    while(*cur){
        cur++;
        rows++;
    }
    total_rows = rows;
    return rows;
}

int getCols(char** newmaze1){
    int cols = 0;
    char **cur = newmaze1;
    char *ptr = *cur;
    while(*ptr != '\0'){
        ptr++;
        cols++;
    }
    total_cols = cols;
    return cols;
}

char **newmaze;

void maze_init(char **template){
	sem_init(&maze_mutex, 0, 1);
	int numRows = getRows(template);
	int numCols = getCols(template);
	newmaze = malloc(sizeof(char*)*numRows);
	time_t t;
	srand((unsigned) time(&t));

	for(int i=0; i<numRows; i++){
		int numCols1 = numCols + 1;
		*(newmaze+i) = malloc(sizeof(char)*numCols1);
		for(int j=0; j<numCols; j++){
			char ch = *(*(template+i)+j);
			*(*(newmaze+i)+j) = ch;
		}
		*(*(newmaze+i)+numCols) = '\0';
	}
}

void maze_fini(){
	int r = total_rows;
	for(int i = 0; i < r; i++)
        free(newmaze[i]);

	free(newmaze);
	sem_destroy(&maze_mutex);
}

int maze_get_rows(){
	return total_rows;
}

int maze_get_cols(){
	return total_cols;
}

int maze_set_player(OBJECT avatar, int row, int col){
	if(row<0 || row>= total_rows || col<0 || col>=maze_get_cols()){
		return 1;
	}
	sem_wait(&maze_mutex);
	if(newmaze[row][col] == ' '){
		newmaze[row][col] = avatar;
		sem_post(&maze_mutex);
		return 0;
	}
	sem_post(&maze_mutex);
	return 1;
}

int maze_set_player_random(OBJECT avatar, int *rowp, int *colp){
	int count = 0;
	sem_wait(&maze_mutex);
	while(count < 100000){
		int r = rand() % total_rows;
		int c = rand() % total_cols;
		if(newmaze[r][c] == ' '){
			newmaze[r][c] = avatar;
			*rowp = r;
			*colp = c;
			sem_post(&maze_mutex);
			return 0;
		}
		count++;
	}
	sem_post(&maze_mutex);
	return 1;
}

void maze_remove_player(OBJECT avatar, int row, int col){
	sem_wait(&maze_mutex);
	newmaze[row][col] = ' ';
	sem_post(&maze_mutex);
}

int maze_move(int row, int col, int dir){
	if(row<0 || row>= total_rows || col<0 || col>=maze_get_cols()){
		return 1;
	}
	sem_wait(&maze_mutex);
	if(dir==0){
		if(row-1 < 0){
			sem_post(&maze_mutex);
			return 1;
		}
		if(newmaze[row-1][col] != ' '){
			sem_post(&maze_mutex);
			return 1;
		}
		newmaze[row-1][col] = newmaze[row][col];
		newmaze[row][col] = ' ';
	}
	else if(dir == 1){
		if(col-1<0){
			sem_post(&maze_mutex);
			return 1;
		}
		if(newmaze[row][col-1] != ' '){
			sem_post(&maze_mutex);
			return 1;
		}
		newmaze[row][col-1] = newmaze[row][col];
		newmaze[row][col] = ' ';
	}
	else if(dir == 2){
		if(row+1 >= total_rows){
			sem_post(&maze_mutex);
			return 1;
		}
		if(newmaze[row+1][col] != ' '){
			sem_post(&maze_mutex);
			return 1;
		}
		newmaze[row+1][col] = newmaze[row][col];
		newmaze[row][col] = ' ';
	}
	else{
		if(col+1>=maze_get_cols()){
			sem_post(&maze_mutex);
			return 1;
		}
		if(newmaze[row][col+1] != ' '){
			sem_post(&maze_mutex);
			return 1;
		}
		newmaze[row][col+1] = newmaze[row][col];
		newmaze[row][col] = ' ';
	}
	sem_post(&maze_mutex);
	return 0;
}

OBJECT maze_find_target(int row, int col, DIRECTION dir){
	sem_wait(&maze_mutex);
	if(dir == 0){
		row--;
		while(row>=0){
			if(isupper(newmaze[row][col])){
				sem_post(&maze_mutex);
				return newmaze[row][col];
			}
			row--;
		}
		sem_post(&maze_mutex);
		return ' ';
	}
	else if(dir == 1){
		col--;
		while(col >= 0){
			if(isupper(newmaze[row][col])){
				sem_post(&maze_mutex);
				return newmaze[row][col];
			}
			col--;
		}
		sem_post(&maze_mutex);
		return ' ';
	}
	else if(dir == 2){
		row++;
		while(row<total_rows){
			if(isupper(newmaze[row][col])){
				sem_post(&maze_mutex);
				return newmaze[row][col];
			}
			row++;
		}
		sem_post(&maze_mutex);
		return ' ';
	}
	else{
		col++;
		while(col < maze_get_cols()){
			if(isupper(newmaze[row][col])){
				sem_post(&maze_mutex);
				return newmaze[row][col];
			}
			col++;
		}
		sem_post(&maze_mutex);
		return ' ';
	}

	sem_post(&maze_mutex);
	return ' ';
}

int maze_get_view(VIEW *view, int row, int col, DIRECTION gaze, int depth){
	sem_wait(&maze_mutex);
	if(gaze == 0){
		int maxdepth = min(depth, row+1);
		int column = col-1;
		int colCount = 0;
		while(column <= col+1){
			int curdepth = 0;
			int r = row;
			while(curdepth < maxdepth){
				*(*(*view+curdepth)+colCount) = newmaze[r][column];
				curdepth++;
				r--;
			}
			column++;
			colCount++;
		}
		sem_post(&maze_mutex);
		return maxdepth;
	}
	else if(gaze == 1){
		int maxdepth = min(depth, col+1);
		int r = row-1;
		int rowCount = 0;
		while(r <= row+1){
			int curdepth = 0;
			int c = col;
			while(curdepth < maxdepth){
				*(*(*view+curdepth)+rowCount) = newmaze[r][c];
				curdepth++;
				c--;
			}
			r++;
			rowCount++;
		}
		sem_post(&maze_mutex);
		return maxdepth;
	}
	else if(gaze == 2){
		int maxdepth = min(depth, total_rows-row);
		int column = col-1;
		int colCount = 0;
		while(column <= col+1){
			int curdepth = 0;
			int r = row;
			while(curdepth < maxdepth){
				*(*(*view+curdepth)+colCount) = newmaze[r][column];
				curdepth++;
				r++;
			}
			column++;
			colCount++;
		}
		sem_post(&maze_mutex);
		return maxdepth;
	}
	else{
		int maxdepth = min(depth, maze_get_cols()-col);
		int r = row-1;
		int rowCount = 0;
		while(r <= row+1){
			int curdepth = 0;
			int c = col;
			while(curdepth < maxdepth){
				*(*(*view+curdepth)+rowCount) = newmaze[r][c];
				curdepth++;
				c++;
			}
			r++;
			rowCount++;
		}
		sem_post(&maze_mutex);
		return maxdepth;
	}
	return 0;
}

void show_view(VIEW *view, int depth){
}

void show_maze(){
	sem_wait(&maze_mutex);
	for(int i=0; i<total_rows; i++){
		fprintf(stderr, "%s\n", *(newmaze+i));
	}
	sem_post(&maze_mutex);
}
