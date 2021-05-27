#ifndef REQUESTS_H
#define REQUESTS_H

typedef struct req{
    int accepted;
    int rejected;
    int total;
}requests;

// This is the entity used in travelStats.
// It refers to a request done for travel on date to a country.
// Each time a request is made for a virus, a virusRequest is either added to the parent's respective hashmap
// and this request is added to its array of named requests, or simply the request is added.
// But since there could be multiple requests for travel on the same date for this virus, we note how many these were,
// how many were accepted and rejected.
// So, when the statistics request comes in, the specific virusRequest is found, and
// if we care about the country, we compare the countryTo field first to said country,
// otherwise just check whether date is in range, and if so add its accepted/rejected/totalRequests
// to the ones being aggregated currently. 

typedef struct namreq{
    char *countryTo;
    char *date; 
    int acceptedOnThisDate;
    int rejectedOnThisDate;
    int totalOnThisDate;
}namedRequests;

void create_namedRequest(namedRequests **, char *, char *);
int isRequestAgainstVirus(namedRequests *, char *);
int isRequestForCountryTo(namedRequests *, char *);
void acceptNamedRequest(namedRequests *);
void rejectNamedRequest(namedRequests *);
void destroy_namedRequest(namedRequests **);
#endif