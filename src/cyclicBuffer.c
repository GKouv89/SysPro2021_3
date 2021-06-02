#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>

#include "../include/cyclicBuffer.h"

void create_cyclicBuffer(cyclicBuffer **cB, int cyclicBufferSize){
    *cB = malloc(sizeof(cyclicBuffer));
    (*cB)->start = 0;
    (*cB)->end = -1;
    (*cB)->count = 0;
    (*cB)->cyclicBufferSize = cyclicBufferSize;
    (*cB)->filePaths = malloc(cyclicBufferSize*sizeof(char *));
    for(int i = 0; i < cyclicBufferSize; i++){
        (*cB)->filePaths[i] = NULL; 
    }
}

void place(cyclicBuffer *cB, char *path, pthread_mutex_t *mtx, pthread_cond_t *cond_nonfull){
    pthread_mutex_lock(mtx);
    while(cB->count >= cB->cyclicBufferSize) {
        printf(">> Found Buffer Full \n");
        pthread_cond_wait(cond_nonfull, mtx);
    }
    cB->end = (cB->end + 1) % cB->cyclicBufferSize;
    if(cB->filePaths[cB->end] != NULL){
        free(cB->filePaths[cB->end]);
    }
    cB->filePaths[cB->end] = malloc((strlen(path) + 1)*sizeof(char));
    strcpy(cB->filePaths[cB->end], path);
    cB->count++;
    pthread_mutex_unlock(mtx);
}

void obtain(cyclicBuffer *cB, pthread_mutex_t *mtx, pthread_cond_t *cond_nonempty, int *hasThreadFinished){
    pthread_mutex_lock(mtx);
    while (cB->count <= 0) {
        // printf(">> Found Buffer Empty \n");
        pthread_cond_wait(cond_nonempty, mtx);
    }
    printf("Consumed path: %s\n", cB->filePaths[cB->start]);
    if(strcmp(cB->filePaths[cB->start], "END") == 0){
        *hasThreadFinished = 1;
    }
    cB->start = (cB->start + 1) % cB->cyclicBufferSize;
    cB->count--;
    pthread_mutex_unlock(mtx);
}

void producer(char **argv, cyclicBuffer *cB, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, pthread_mutex_t *mtx){
    DIR *work_dir;
	struct dirent *curr_subdir;
    char *full_file_name = malloc(1024*sizeof(char));
    for(int i = 11; argv[i] != NULL; i++){
        work_dir = opendir(argv[i]);
		curr_subdir = readdir(work_dir);
        while(curr_subdir != NULL){
            if(strcmp(curr_subdir->d_name, ".") == 0 || strcmp(curr_subdir->d_name, "..") == 0){
				curr_subdir = readdir(work_dir);
				continue;
			}
            strcpy(full_file_name, argv[i]);
            // strcat(full_file_name, "/");
            strcat(full_file_name, curr_subdir->d_name);
            place(cB, full_file_name, mtx, cond_nonfull);
            pthread_cond_signal(cond_nonempty);
    		curr_subdir = readdir(work_dir);
        }
		closedir(work_dir);
    }
    free(full_file_name);
}

void killThreadPool(int numThreads, cyclicBuffer *cB, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, pthread_mutex_t *mtx){
    char *endStr = "END";
    for(int i = 0; i < numThreads; i++){
        place(cB, endStr, mtx, cond_nonfull);
        pthread_cond_signal(cond_nonempty);
    }
}

void *consumer(void * ptr){
    consumerThreadArgs *args = ptr;
    while(args->cB->count > 0 || !args->hasThreadFinished){
        obtain(args->cB, args->mtx, args->cond_nonempty, &(args->hasThreadFinished));
        pthread_cond_signal(args->cond_nonfull);
        if(args->hasThreadFinished){
            break;
        }
    }
    pthread_exit(0);
}

void destroy_cyclicBuffer(cyclicBuffer **cB){
    for(int i = 0; i < (*cB)->cyclicBufferSize; i++){
        free((*cB)->filePaths[i]);
    }
    free((*cB)->filePaths);
    free(*cB);
}



