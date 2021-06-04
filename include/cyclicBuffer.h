#ifndef CYCLICBUFFER_H
#define CYCLICBUFFER_H

#include <pthread.h>

#include "hashmap.h"

typedef struct {
    char **filePaths;
    int start;
    int end;
    int count;
    int cyclicBufferSize;
} cyclicBuffer;

typedef struct {
    cyclicBuffer *cB;
    pthread_mutex_t *mtx;
    pthread_mutex_t *dataStructAccs;
    pthread_cond_t *cond_nonempty;
    pthread_cond_t *cond_nonfull;
    pthread_cond_t *allfiles_consumed;
    int hasThreadFinished;
    hashMap *virus_map;
    hashMap *country_map;
    hashMap *citizen_map;
    int sizeOfBloom;
    int *filesConsumed;
} consumerThreadArgs;

void create_cyclicBuffer(cyclicBuffer **, int);
void place(cyclicBuffer *, char *, pthread_mutex_t *, pthread_cond_t *);
void obtain(cyclicBuffer *, pthread_mutex_t *, pthread_cond_t *, int *, int, pthread_mutex_t *, hashMap *, hashMap *, hashMap *, int *);
void producer(char **, cyclicBuffer *, pthread_cond_t *, pthread_cond_t *, pthread_mutex_t *, int *);
void addNewRecords(Country *, char *, cyclicBuffer *, pthread_cond_t *, pthread_cond_t *, pthread_mutex_t *, int *);
void killThreadPool(int, cyclicBuffer *, pthread_cond_t *, pthread_cond_t *, pthread_mutex_t *);
void *consumer(void *);
void destroy_cyclicBuffer(cyclicBuffer **);

#endif