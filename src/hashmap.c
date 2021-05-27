#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/hashmap.h"
#include "../include/bloomfilter.h"
#include "../include/readWriteOps.h"


void create_map(hashMap **map, int noOfBuckets, typeOfList type){
  (*map) = malloc(sizeof(hashMap));
  (*map)->noOfBuckets = noOfBuckets;
  (*map)->map = malloc(noOfBuckets*sizeof(hashBucket *));
  for(int i = 0; i < (*map)->noOfBuckets; i++){
    (*map)->map[i] = malloc(sizeof(hashBucket));
    create_bucketList(&((*map)->map[i]->bl), type);
  }
}

unsigned long hash_function(hashMap *map, unsigned char *str){
  unsigned long hash = djb2(str);
  return hash % map->noOfBuckets;
}

void insert(hashMap *map, unsigned char *key, void *content){
  unsigned long hash = hash_function(map, key);
  insert_bucketNode(map->map[hash]->bl, content);
}

void* find_node(hashMap *map, unsigned char *key){
  unsigned long hash = hash_function(map, key);
  search_bucketList(map->map[hash]->bl, key);
}

/////////////////////////////////////////////////////////////////
// This function is called only with the viruses hashmap       //
// for the first argument. Basically searches for each element //
// in each bucket, i.e. each virus, and sends the virus'       //
// bloom filter to the parent.                                 //
/////////////////////////////////////////////////////////////////

void send_bloomFilters(hashMap *map, int readfd, int writefd, int bufferSize){
    for(int i = 0; i < map->noOfBuckets; i++){
      send_virus_Bloomfilters(map->map[i]->bl, readfd, writefd, bufferSize);
    }
    // No more filters to send
    char *endStr = "END";
    unsigned int charsCopied, endStrLen = 3;
    char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
    if(write(writefd, &endStrLen, sizeof(int)) < 0){
      perror("write END length");
    }else{ 
      charsCopied = 0;
      while(charsCopied < endStrLen){
        strncpy(pipeWriteBuffer, endStr + charsCopied, bufferSize);
        if(write(writefd, pipeWriteBuffer, bufferSize) < 0){
          perror("write END chunk");
        }
        charsCopied += bufferSize;
      }
    }
    free(pipeWriteBuffer);
}

// This function is only called with the country map as a first argument, and only when about to respawn a child.
// The bucket lists of the parent are traversed, and every country whose index matches the index of the child
// that died abruptly, is added to the ctounries array, and then they're all sent to the new child.

void sendCountryNamesToChild(hashMap *map, int readfd, int writefd, int bufferSize, int monitorIndex){
  char **countries = malloc(250*sizeof(char *));
  int countryIndex = 0;
  for(int i = 0; i < map->noOfBuckets; i++){
    findCountriesForChild(map->map[i]->bl, &countries, &countryIndex, monitorIndex);
  }
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));  
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  unsigned int countryLength, charsCopied, charsToWrite;
  for(int i = 0; i < countryIndex; i++){
    countryLength = strlen(countries[i]);
    if(write(writefd, &countryLength, sizeof(int)) < 0){
      perror("write country length to new child");
    }else{
      charsCopied = 0;
      while(charsCopied < countryLength){
        if(countryLength - charsCopied < bufferSize){
          charsToWrite = countryLength - charsCopied;
        }else{
          charsToWrite = bufferSize;
        }
        strncpy(pipeWriteBuffer, countries[i] + charsCopied, charsToWrite*sizeof(char));
        if(write(writefd, pipeWriteBuffer, charsToWrite) < 0){
          perror("write country chunk to new child");
        }
        charsCopied += charsToWrite;
      }
      while(read(readfd, pipeReadBuffer, bufferSize) < 0);
    }
  }
  // NO more countries to send
  char *endStr = "END";
  unsigned int endStrLen = 3;
  if(write(writefd, &endStrLen, sizeof(int)) < 0){
    perror("write END length");
  }else{
    charsCopied = 0;
    while(charsCopied < endStrLen){
      if(endStrLen - charsCopied < bufferSize){
        charsToWrite = endStrLen - charsCopied;
      }else{
        charsToWrite = bufferSize;
      }
      strncpy(pipeWriteBuffer, endStr + charsCopied, charsToWrite*sizeof(char));
      if(write(writefd, pipeWriteBuffer, charsToWrite*sizeof(char)) < 0){
        perror("write END chunk");
      }
      charsCopied += charsToWrite;
    }
  }
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
  for(int i = 0; i < countryIndex; i++){
    free(countries[i]);
  }
  free(countries);
}

// Used in log file creation.

void printSubdirectoryNames(hashMap *map, FILE *fp){
  for(int i = 0; i < map->noOfBuckets; i++){
    printSubdirNames(map->map[i]->bl, fp);
  }  
}

/////////////////////////////////////////////////////////////////
// This function is called only with the viruses hashmap       //
// for the first argument. Basically searches for each element //
// in each bucket, i.e. each virus, whether the citizen        //
// has been vaccinated for it and if so, when.                 //
// For each vaccination (or lack thereof) of the citizen, a    //
// response is forwarded to the parent.                        //
/////////////////////////////////////////////////////////////////

void lookup_vacStatus_all(hashMap *map, unsigned char *citizenID, int readfd, int writefd, int bufferSize){
  for(int i = 0; i < map->noOfBuckets; i++){
    vacStatus_all(map->map[i]->bl, citizenID, readfd, writefd, bufferSize);
  }
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char *endStr = "END";
  write_content(endStr, &pipeWriteBuffer, writefd, bufferSize);
  free(pipeWriteBuffer);
}

void destroy_map(hashMap **map){
  for(int i = 0; i < (*map)->noOfBuckets; i++){
    destroy_bucketList(&((*map)->map[i]->bl));
    free((*map)->map[i]);
    (*map)->map[i] = NULL;
  }
  free((*map)->map);
  (*map)->map = NULL;
  free(*map);
  *map = NULL;
}
