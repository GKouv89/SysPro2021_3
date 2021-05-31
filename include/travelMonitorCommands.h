#ifndef TRAVELMONITORCLIENTCOMMANDS_H
#define TRAVELMONITORCLIENTCOMMANDS_H

#include "../include/requests.h"

void receiveBloomFiltersFromChild(hashMap *, int, int, int, int, int);
void travelRequest(hashMap *, hashMap *, hashMap *, char *, char *, char *, char *, char *, int, int *, requests *);

#endif