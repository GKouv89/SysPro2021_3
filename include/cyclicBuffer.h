#ifndef CYCLICBUFFER_H
#define CYCLICBUFFER_H

#include <pthread.h>

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
    pthread_cond_t *cond_nonempty;
    pthread_cond_t *cond_nonfull;
    int hasThreadFinished;
} consumerThreadArgs;

void create_cyclicBuffer(cyclicBuffer **, int);
void place(cyclicBuffer *, char *, pthread_mutex_t *, pthread_cond_t *);
void obtain(cyclicBuffer *, pthread_mutex_t *, pthread_cond_t *, int *);
void producer(char **, cyclicBuffer *, pthread_cond_t *, pthread_cond_t *, pthread_mutex_t *);
void killThreadPool(int, cyclicBuffer *, pthread_cond_t *, pthread_cond_t *, pthread_mutex_t *);
void *consumer(void *);
void destroy_cyclicBuffer(cyclicBuffer **);

#endif