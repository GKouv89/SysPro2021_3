#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "../include/setofbfs.h"

void create_setOfBFs(setofbloomfilters **set, char *virusName, int arrayCount, int sizeOfBloom){
    (*set) = malloc(sizeof(setofbloomfilters));
    (*set)->capacity = arrayCount;
    (*set)->sizeOfBloom = sizeOfBloom;
    (*set)->bfs = malloc(arrayCount*sizeof(bloomFilter *));
    for(int i = 0; i < arrayCount; i++){
        (*set)->bfs[i] = NULL;
    }
    (*set)->virusName = malloc((strlen(virusName) + 1)*sizeof(char));
    strcpy((*set)->virusName, virusName);
}

int isEqual_setOfBFs(setofbloomfilters *set, unsigned char *virusName){
    if(strcmp(set->virusName, virusName) == 0){
        return 1;
    }
    return 0;
}

void add_BFtoSet(setofbloomfilters *set, int index){
    // when select chooses a child to receive the filter from,
    // the index is the same as the index of the pipe's file descriptor
    // in the read and write file descriptor arrays of the parent.
    if(set->bfs[index] != NULL){
        // in this case, the filter is being replaced, either by 
        // an update in the subdirectories of a child process, or
        // by the replacement of the child process itself
        destroy_bloomFilter(&(set->bfs[index]));
        set->bfs[index] = NULL;
    }
    create_bloomFilter(&(set->bfs[index]), set->sizeOfBloom, 16);
}

int lookup_bf_vaccination(setofbloomfilters *set, int index, unsigned char *str){
    // A check, in case there is a false countryFrom, or a correct one but a virus
    // we have no records for and techinically also no bloom filter for.
    if(set->bfs[index] == NULL){
        return 0;
    }
    return lookup_in_bloomFilter(set->bfs[index], str);
}

void read_BF(setofbloomfilters *set, int readfd, int writefd, int index, int bufferSize){
    char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
    char pipeWriteBuffer;
    int bytesRead;
    unsigned int bytesParsed = 0;
    int sizeOfBloom = (set->bfs[index]->size)/8;
    while(bytesParsed < sizeOfBloom){
        if((bytesRead = read(readfd, pipeReadBuffer, bufferSize*sizeof(char))) < 0){
            // This just means that the child process hasn't written the next chunk yet, but will soon.
            continue;
        }else{
            memcpy(set->bfs[index]->filter + bytesParsed, pipeReadBuffer, bytesRead*sizeof(char));
            bytesParsed += bytesRead;
        }
    }
    pipeWriteBuffer = '1';
    if(write(writefd, &pipeWriteBuffer, sizeof(char)) < 0){
        perror("write bf confirmation\n");
    }
    free(pipeReadBuffer);
}

void destroy_setOfBFs(setofbloomfilters **set){
    for(int i = 0; i < (*set)->capacity; i++){
        if((*set)->bfs[i] != NULL){
            destroy_bloomFilter(&((*set)->bfs[i]));
            (*set)->bfs[i] = NULL;
        }
    }
    free((*set)->bfs);
    free((*set)->virusName);
    free(*set);
    *set = NULL;
}