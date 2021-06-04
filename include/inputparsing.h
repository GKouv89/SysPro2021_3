#ifndef INPUTPARSING_H
#define INPUTPARSING_H

#include <pthread.h>

#include "hashmap.h"

void inputFileParsing(hashMap *, hashMap *, hashMap *, FILE *, int, pthread_mutex_t *);

#endif