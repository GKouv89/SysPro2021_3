#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/readWriteOps.h"
#include "../include/hashmap.h"
#include "../include/setofbfs.h"
#include "../include/requests.h"
#include "../include/virusRequest.h"
#include "../include/dateOps.h"

void receiveBloomFiltersFromChild(hashMap *setOfBFs_map, int sock_id, int index, int bufferSize, int numMonitors, int sizeOfBloom){
    unsigned int dataLength, charactersParsed;
    int charactersRead;
    char *virusName = calloc(255, sizeof(char));
    char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
    char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
    setofbloomfilters *curr_set;
    while(1){
        read_content(&virusName, &pipeReadBuffer, sock_id, bufferSize);
        if(strcmp(virusName, "END") == 0){
            // no more bloom filters from this child.
            break;
        }
        curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
        if(curr_set == NULL){
            // completely new virus to parent
            create_setOfBFs(&curr_set, virusName, numMonitors, sizeOfBloom);
            add_BFtoSet(curr_set, index);
            insert(setOfBFs_map, virusName, curr_set);
        }else{
            add_BFtoSet(curr_set, index);
        }
        if(write(sock_id, "1", sizeof(char)) < 0){
            perror("write confirmation after receiving virus name");
        }
        // Reading actual array/filter
        read_BF(curr_set, sock_id, index, bufferSize);
        memset(virusName, 0, 255*sizeof(char));
    }
    free(virusName);
    free(pipeReadBuffer);
    free(pipeWriteBuffer);
}

void travelRequest(hashMap *setOfBFs_map, hashMap *country_map, hashMap *virusRequest_map, char *citizenID, char *dateOfTravel, char *countryName, char *countryTo, char *virusName, int bufferSize, int *sock_ids, requests *reqs){
    Country *curr_country;
	setofbloomfilters *curr_set = (setofbloomfilters *) find_node(setOfBFs_map, virusName);
    if(curr_set == NULL){
        printf("No such virus: %s\n", virusName);
        return;
    }else{
        // Have we received any requests for this virus yet? If not, time
        // to create the appropriate 'object' 
        virusRequest *vr = (virusRequest *) find_node(virusRequest_map, virusName);
        if(vr == NULL){
            create_virusRequest(&vr, virusName);
            insert(virusRequest_map, virusName, vr);
        }
        // Now that we definitely have a virusRequest, let's see if there have been any
        // other requests for travel to the same country on the same date. If not, 
        // let's create a namedRequest. addRequest does both the check and (if needed) the creation.
        namedRequests *nreq = addRequest(vr, countryTo, dateOfTravel);
        // Find which monitor handled the country's subdirectory
        curr_country = (Country *) find_node(country_map, countryName);
        if(curr_country == NULL){
            printf("No such country: %s\n", countryName);
            return;
        }
        if(lookup_bf_vaccination(curr_set, curr_country->index, citizenID) == 1){
            char *request = malloc(1024*sizeof(char));
            char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
            char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
            unsigned int request_length;
            sprintf(request, "checkSkiplist %s %s %s", citizenID, virusName, countryName);
            // sending request to child 
            write_content(request, &pipeWriteBuffer, sock_ids[curr_country->index], bufferSize);
            memset(request, 0, 1024*sizeof(char));
            // reading response from child
            read_content(&request, &pipeReadBuffer, sock_ids[curr_country->index], bufferSize);
            // Send confirmation of answer reception.
            if(write(sock_ids[curr_country->index], "1", sizeof(char)) < 0){
                perror("write answer reception confirmation");
            }
            request_length = strlen(request);
            if(request_length == 2){
                printf("REQUEST REJECTED - YOU ARE NOT VACCINATED\n");
                reqs->rejected++;
                reqs->total++;
            }else if(strcmp(request, "BAD COUNTRY") == 0){
                printf("REQUEST REJECTED - INVALID COUNTRYFROM ARGUMENT FOR CITIZEN %s\n", citizenID);
                // This is NOT counted towards the overall requests, as the arguments were invalid.
            }else{
                char *answer = malloc(4*sizeof(char));
                char *date = malloc(12*sizeof(char));
                sscanf(request, "%s %s", answer, date);
                memset(request, 0, 1024*sizeof(char));
                // Parent will now check if the day of the trip was withing six months of the date of vaccination
                switch(dateDiff(dateOfTravel, date)){
                case -1: printf("ERROR - TRAVEL DATE BEFORE VACCINATION DATE\n");
                    strcpy(request, "REJECT");
                    // letting child know that request was rejected
                    write_content(request, &pipeWriteBuffer, sock_ids[curr_country->index], bufferSize);
                    rejectNamedRequest(nreq);
                    reqs->rejected++;
                    reqs->total++;
                    break;
                case 0: printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
                    strcpy(request, "ACCEPT");  
                    // letting child know that request was accepted
                    write_content(request, &pipeWriteBuffer, sock_ids[curr_country->index], bufferSize);
                    acceptNamedRequest(nreq);
                    reqs->accepted++;
                    reqs->total++;
                    break;
                case 1: printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
                    strcpy(request, "REJECT");
                    // letting child know that request was rejected
                    write_content(request, &pipeWriteBuffer, sock_ids[curr_country->index], bufferSize);
                    rejectNamedRequest(nreq);
                    reqs->rejected++;
                    reqs->total++;
                    break;
                default: printf("Invalid return value from date difference function.\n");
                    break; 
                }
                free(answer);
                free(date);
            }
            free(request);
            free(pipeReadBuffer);
            free(pipeWriteBuffer);
        }else{
            // Just rejecting the request, no need to ask the child
            rejectNamedRequest(nreq);
            reqs->rejected++;
            reqs->total++;
            printf("REQUESTED REJECTED - YOU ARE NOT VACCINATED\n");
        }
    } 
}