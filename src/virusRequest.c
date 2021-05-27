#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/virusRequest.h"
#include "../include/dateOps.h"

void create_virusRequest(virusRequest **vr, char *virusName){
    *vr = malloc(sizeof(virusRequest));
    (*vr)->virusName = malloc((strlen(virusName) + 1)*sizeof(char));
    strcpy((*vr)->virusName, virusName);
    (*vr)->capacity = 10;
    (*vr)->requests = malloc((*vr)->capacity*sizeof(namedRequests *));
    (*vr)->length = 0;
}

int isEqual_virusRequest(virusRequest *vr, char *str){
    if(strcmp(vr->virusName, str) == 0){
        return 1;
    }
    return 0;
}

void resize_virusRequest(virusRequest *vr){
    vr->capacity *= 2;
    namedRequests **temp = realloc(vr->requests, vr->capacity*sizeof(namedRequests *));
    assert(temp != NULL);
    vr->requests = temp;
}

// Checks to see if a named request exists. If not, it is created. In both cases,
// a pointer is returned, as we will most likely need to accept or reject it.

namedRequests* addRequest(virusRequest *vr, char *countryTo, char *date){
    namedRequests *nreq;
    if((nreq = findRequest(vr, countryTo, date)) != NULL)
        return nreq;
    namedRequests *new;
    create_namedRequest(&new, countryTo, date);
    vr->requests[vr->length] = new;
    vr->length++;
    if(vr->length == vr->capacity){
        resize_virusRequest(vr);
    }
    return new;
}

namedRequests* findRequest(virusRequest *vr, char *countryTo, char *date){
    for(int i = 0; i < vr->length; i++){
        if(strcmp(vr->requests[i]->countryTo, countryTo) == 0 && strcmp(vr->requests[i]->date, date) == 0){
            return vr->requests[i];
        }
    }
    return NULL;
}

// Mode == 1: we care for a specific countryTo. Traversing the vector, taking into account
// only the requests about this specific country, we sum the total, accepted and rejected requests
// for the parent to print out after /travelStats was issued in a specific date range.
// Mode == 0: All requests are aggregated, no matter the country. 

void gatherStatistics(virusRequest *vr, char *date1, char *date2, char *countryTo, int mode, requests *reqs){
    for(int i = 0; i < vr->length; i++){
        if(mode){
            if(isRequestForCountryTo(vr->requests[i], countryTo)){
                if(!dateComparison(date1, vr->requests[i]->date) && !dateComparison(vr->requests[i]->date, date2)){
                    reqs->accepted += vr->requests[i]->acceptedOnThisDate;
                    reqs->rejected += vr->requests[i]->rejectedOnThisDate;
                    reqs->total += vr->requests[i]->totalOnThisDate;
                }
            }
        }else{
            // Straight to date comparison, we don't care for the country.
            if(!dateComparison(date1, vr->requests[i]->date) && !dateComparison(vr->requests[i]->date, date2)){
                reqs->accepted += vr->requests[i]->acceptedOnThisDate;
                reqs->rejected += vr->requests[i]->rejectedOnThisDate;
                reqs->total += vr->requests[i]->totalOnThisDate;
            }
        }
    }
}

void destroy_virusRequest(virusRequest **vr){
    free((*vr)->virusName);
    for(int i = 0; i < (*vr)->length; i++){
        destroy_namedRequest(&((*vr)->requests[i]));
    }
    free((*vr)->requests);
    free(*vr);
}

