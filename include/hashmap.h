#ifndef HASHMAP_H
#define HASHMAP_H

#include "bucketlist.h"

typedef struct bucket{
  bucketList* bl;
}hashBucket;

typedef struct map{
  int noOfBuckets;
  hashBucket **map;
}hashMap;

void create_map(hashMap **, int, typeOfList);
unsigned long hash_function(hashMap*, unsigned char*);
void insert(hashMap*, unsigned char*, void *);
void* find_node(hashMap*, unsigned char*);
void send_bloomFilters(hashMap *, int, int, int);
void sendCountryNamesToChild(hashMap *, int, int, int, int);
void printSubdirectoryNames(hashMap *, FILE *);
void lookup_vacStatus_all(hashMap *, unsigned char *, int, int, int);
void destroy_map(hashMap**);
#endif
