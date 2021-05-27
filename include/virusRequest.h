#ifndef VIRUSREQUEST_H
#define VIRUSREQUEST_H

#include "requests.h"

// A vector of sorts that holds entities of type namedRequests
// when a travelRequest is about a specific date, countryTo and about this virus,
// an element is added to the array.

typedef struct virreq{
    char *virusName;
    namedRequests **requests;
    int capacity;
    int length;
}virusRequest;

void create_virusRequest(virusRequest **, char *);
int isEqual_virusRequest(virusRequest *, char *);
void resize_virusRequest(virusRequest *);
namedRequests* addRequest(virusRequest *, char *, char *);
namedRequests* findRequest(virusRequest *, char *, char *);
void gatherStatistics(virusRequest *, char *, char *, char *, int, requests *);
void destroy_virusRequest(virusRequest **);
#endif
