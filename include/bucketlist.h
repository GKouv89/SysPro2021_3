#ifndef BUCKETLIST_H
#define BUCKETLIST_H

#include "virus.h"

typedef enum tol{Country_List, Virus_List, Citizen_List, BFLookup_List, VirusRequest_List} typeOfList;

typedef struct bucketlistnode{
  void *content;
  struct bucketlistnode *next;
} bucketNode;

typedef struct bucketlist{
  typeOfList type;
  bucketNode *front;
  bucketNode *rear;
} bucketList;

void create_bucketList(bucketList **, typeOfList);
void insert_bucketNode(bucketList *, void *);
void print_bucketList(bucketList *);
void* search_bucketList(bucketList *, char *);
void send_virus_Bloomfilters(bucketList *, int, int, int);
void findCountriesForChild(bucketList *, char ***, int *, int);
void printSubdirNames(bucketList *bl, FILE *);
void vacStatus_all(bucketList *, unsigned char *, int, int, int);
void destroy_bucketList(bucketList **);

#endif
