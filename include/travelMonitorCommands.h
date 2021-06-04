#ifndef TRAVELMONITORCLIENTCOMMANDS_H
#define TRAVELMONITORCLIENTCOMMANDS_H

#include "../include/requests.h"

void receiveBloomFiltersFromChild(hashMap *, int, int, int, int, int);
void travelRequest(hashMap *, hashMap *, hashMap *, char *, char *, char *, char *, char *, int, int *, requests *);
void searchVaccinationStatus(int *, int, int, char *);
void addVaccinationRecords(hashMap *, hashMap *, char *, char *, int *, int, int, int);

#endif