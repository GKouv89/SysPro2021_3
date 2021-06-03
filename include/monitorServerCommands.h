#ifndef MONITORSERVERCOMMANDS_H
#define MONITORSERVERCOMMANDS_H

#include "../include/requests.h"

void checkSkiplist(hashMap *, char *, char *, int, int, requests *);
void checkVacc(hashMap *, hashMap *, char *, int, int);

#endif