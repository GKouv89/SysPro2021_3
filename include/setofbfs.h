#ifndef SETOFBFS_H
#define SETOFBFS_H

#include "bloomfilter.h"

typedef struct setofbfs{
    char *virusName;
    // An array of bloomfilters on virusName, used for lookup.
    // Each index corresponds to a monitor, a.k.a.
    // bfs[0] contains the filter for the countries
    // handled by monitorProcess 0 for this virus, and so forth.
    int capacity; // how many bloom filters could be, granted that every child has records on this virus
    // this is why we initialize the elements of the bfs array as NULL; maybe a child doesn't have a bloom filter for this
    // virus, but in any case the array has size equal to the value of capacity.
    int sizeOfBloom; // same for all filters
    bloomFilter **bfs;
} setofbloomfilters;

void create_setOfBFs(setofbloomfilters **, char *, int, int);
int isEqual_setOfBFs(setofbloomfilters *, unsigned char *);
void add_BFtoSet(setofbloomfilters *set, int);
int lookup_bf_vaccination(setofbloomfilters *, int, unsigned char *);
void read_BF(setofbloomfilters *, int, int, int, int);
void destroy_setOfBFs(setofbloomfilters **);
#endif