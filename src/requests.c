#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/requests.h"

void create_namedRequest(namedRequests **nreqs, char *countryTo, char *date){
    (*nreqs) = malloc(sizeof(namedRequests));
    (*nreqs)->countryTo = malloc((strlen(countryTo) + 1)*sizeof(char));
    (*nreqs)->date = malloc((strlen(date) + 1)*sizeof(char));
    strcpy((*nreqs)->countryTo, countryTo);
    strcpy((*nreqs)->date, date);
    (*nreqs)->totalOnThisDate = 0;
    (*nreqs)->acceptedOnThisDate = 0;
    (*nreqs)->rejectedOnThisDate = 0;
}

int isRequestForCountryTo(namedRequests *nreqs, char *countryName){
    if(strcmp(nreqs->countryTo, countryName) == 0){
        return 1;
    }
    return 0;
}

void acceptNamedRequest(namedRequests *nreqs){
    nreqs->acceptedOnThisDate++;
    nreqs->totalOnThisDate++;
}
void rejectNamedRequest(namedRequests *nreqs){
    nreqs->rejectedOnThisDate++;
    nreqs->totalOnThisDate++;
}

void destroy_namedRequest(namedRequests **nreqs){
    free((*nreqs)->countryTo);
    free((*nreqs)->date);
    free(*nreqs);
}