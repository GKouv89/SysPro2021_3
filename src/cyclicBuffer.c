#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "../include/cyclicBuffer.h"
#include "../include/hashmap.h"
#include "../include/inputparsing.h"
#include "../include/country.h"

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
        // printf(">> Found Buffer Full \n");
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

void obtain(cyclicBuffer *cB, pthread_mutex_t *mtx, pthread_cond_t *cond_nonempty, int *hasThreadFinished, int sizeOfBloom, pthread_mutex_t *dataStructAccs, hashMap *country_map, hashMap *citizen_map, hashMap *virus_map, int *filesConsumed){
    FILE *fp;
    char *file_name = malloc(512*sizeof(char));
    pthread_mutex_lock(mtx);
    while (cB->count <= 0) {
        // printf(">> Found Buffer Empty \n");
        pthread_cond_wait(cond_nonempty, mtx);
    }
    strcpy(file_name, cB->filePaths[cB->start]);
    cB->start = (cB->start + 1) % cB->cyclicBufferSize;
    cB->count--;
    pthread_mutex_unlock(mtx);
    if(strcmp(file_name, "END") == 0){
        *hasThreadFinished = 1;
        free(file_name);
        return;
    }
    // About to see if it is necessary to add country to hashmap 
    // to start counting the number of files in that folder.
    char *argument, *big_folder_name, *little_folder_name, *rest;
    argument = malloc((strlen(file_name) + 1)*sizeof(char));
    strcpy(argument, file_name);
    big_folder_name = strtok_r(argument, "/", &rest);
    little_folder_name = strtok_r(NULL, "/", &rest);
    pthread_mutex_lock(dataStructAccs);
    Country *country;
    if((country = (Country *) find_node(country_map, little_folder_name)) == NULL){
        country = create_country(little_folder_name, -1);
        insert(country_map, little_folder_name, country);
    }
    pthread_mutex_unlock(dataStructAccs);
    free(argument);
    // Here, inputFileParsing will be called, with a seperate mutex that denotes 
    // access to the bloom filters and skiplists.
    fp = fopen(file_name, "r");
    assert(file_name != NULL);
    // printf("About to parse: %s\n", file_name);
    inputFileParsing(country_map, citizen_map, virus_map, fp, sizeOfBloom, dataStructAccs);
    // printf("Done parsing: %s\n", file_name);
    pthread_mutex_lock(dataStructAccs);
    readCountryFile(country);
    (*filesConsumed)++;
    printf("Files consumed: %d\n", *filesConsumed);
    pthread_mutex_unlock(dataStructAccs);
    assert(fclose(fp) == 0);
    free(file_name);
}

void producer(char **argv, cyclicBuffer *cB, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, pthread_mutex_t *mtx, int *filesProduced){
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
            (*filesProduced)++;
            pthread_cond_signal(cond_nonempty);
    		curr_subdir = readdir(work_dir);
        }
		closedir(work_dir);
    }
    free(full_file_name);
}

// Used specifically for one folder in addVaccinationRecords
void addNewRecords(Country *country, char *folderName, cyclicBuffer *cB, pthread_cond_t *cond_nonempty, pthread_cond_t *cond_nonfull, pthread_mutex_t *mtx, int *filesProduced){
    DIR *work_dir;
	struct dirent *curr_subdir;
    work_dir = opendir(folderName);
    curr_subdir = readdir(work_dir);
    char *fileName = malloc(1024*sizeof(char));
    while(1){
        sprintf(fileName, "%s/%s-%d.txt", folderName, country->name, country->maxFile + 1);
        FILE *fp = fopen(fileName, "r");
        if(fp == NULL){
            // no more files to read
            break;
        }
        place(cB, fileName, mtx, cond_nonfull);
        printf("Produced: %s\n", fileName);
        readCountryFile(country);
        assert(fclose(fp) == 0);
        (*filesProduced)++;
        pthread_cond_signal(cond_nonempty);
        memset(fileName, 0, 1024*sizeof(char));			
    }
    closedir(work_dir);    
    free(fileName);
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
        obtain(args->cB, args->mtx, args->cond_nonempty, &(args->hasThreadFinished), args->sizeOfBloom, args->dataStructAccs, args->country_map, args->citizen_map, args->virus_map, args->filesConsumed);
        pthread_cond_signal(args->cond_nonfull);
        pthread_cond_signal(args->allfiles_consumed);
        if(args->hasThreadFinished){
            break;
        }
    }
    return NULL;
}

void destroy_cyclicBuffer(cyclicBuffer **cB){
    for(int i = 0; i < (*cB)->cyclicBufferSize; i++){
        free((*cB)->filePaths[i]);
    }
    free((*cB)->filePaths);
    free(*cB);
}



