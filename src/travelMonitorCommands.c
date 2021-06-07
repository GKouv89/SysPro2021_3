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
                rejectNamedRequest(nreq);
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

void searchVaccinationStatus(int *sock_ids, int numMonitors, int bufferSize, char *citizenID){
  unsigned int commandLength, charsWritten, charsToWrite;
  char *command = malloc(15*sizeof(char));
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char confirmation;
  // Sending a request to ALL monitors
  sprintf(command, "checkVacc %s", citizenID);
  commandLength = strlen(command);
  int i;
  for(i = 0; i < numMonitors; i++){
    if(write(sock_ids[i], &commandLength, sizeof(int)) < 0){
      perror("write checkVacc length");
    }else{
      charsWritten = 0;
      while(charsWritten < commandLength){
        if(commandLength - charsWritten < bufferSize){
          charsToWrite = commandLength - charsWritten;
        }else{
          charsToWrite = bufferSize;
        }
        strncpy(pipeWriteBuffer, command + charsWritten, charsToWrite);
        if(write(sock_ids[i], pipeWriteBuffer, charsToWrite*sizeof(char)) < 0){
          perror("write checkVacc command chunk");
        }else{
            charsWritten += charsToWrite;
        }
      }
    }
  }

  // Initializing set of file descriptors. 
  // The monitor that will actually have the citizen will take longer to respond,
  // so with select we can have the others that will respond negatively out of the way first.
  // When the monitor with the citizen is identified, the parent will block until that specific
  // file descriptor has a new line for the parent to print, until there are no more lines (for this reason, rd_specific is used).
  fd_set rd, rd_specific;
  int *valid_read_file_descs = malloc(numMonitors*sizeof(int));
  int max = 0;
  FD_ZERO(&rd);
	for(i = 0; i < numMonitors; i++){
		FD_SET(sock_ids[i], &rd);
        valid_read_file_descs[i] = 1;
        if(sock_ids[i] > max){
            max = sock_ids[i];
        }
	}
  max++;
  char charsCopied, charsRead, virusLength, answerLength;
  char *citizenData = calloc(1024, sizeof(char));
  char *virus = calloc(255, sizeof(char));
  char *answer = calloc(255, sizeof(char));
  char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
  char *ans = malloc(4*sizeof(char));
  char *date = malloc(11*sizeof(char));
  // if a negative answer is received from all monitors,
  // this remains 0 and the user is notified via the parent.
  // if not, this gets 1 assigned.
  int citizenFound = 0;
  for(i = 0; i < numMonitors; ){
    if(select(max, &rd, NULL, NULL, NULL) == -1){
      perror("select checkVacc response");
    }else{
      for(int j = 0; j < numMonitors; j++){
        if(valid_read_file_descs[j] == 1 && FD_ISSET(sock_ids[j], &rd)){
          read_content(&citizenData, &pipeReadBuffer, sock_ids[j], bufferSize);
          // Negative answer
          if(strcmp(citizenData, "NO SUCH CITIZEN") == 0){
            memset(citizenData, 0, 255*sizeof(char));  
          }else{
            printf("%s", citizenData);
            if(write(sock_ids[j], "1", sizeof(char)) < 0){
              perror("couldn't confirm reception of citizenData in searchVaccinationStatus");
            }
            while(1){
              read_content(&virus, &pipeReadBuffer, sock_ids[j], bufferSize);
              // No more viruses the child has info on
              if(strcmp(virus, "END") == 0){
                break;
              }
              citizenFound = 1;
              if(write(sock_ids[j], "1", sizeof(char)) < 0){
                perror("couldn't confirm reception of virus in searchVaccinationStatus");
              }else{
                // wish to block only to wait trafic from this specific monitor 
                FD_ZERO(&rd_specific);
                FD_SET(sock_ids[j], &rd_specific);
                if(select(sock_ids[j] + 1, &rd_specific, NULL, NULL, NULL) == -1){
                  perror("select checkVacc answer reception");
                }else{
                  read_content(&answer, &pipeReadBuffer, sock_ids[j], bufferSize);
                  if(strlen(answer) == 2){
                    printf("%s NOT YET VACCINATED\n", virus);
                  }else{
                    sscanf(answer, "%s %s", ans, date);
                    printf("%s VACCINATED ON %s\n", virus, date);
                  }
                  memset(virus, 0, 255*sizeof(char));
                  memset(answer, 0, 255*sizeof(char));
                  memset(citizenData, 0, 1024*sizeof(char));
                  if(write(sock_ids[j], "1", sizeof(char)) < 0){
                    perror("couldn't confirm reception of answer in searchVaccinationStatus");
                  }
                }
              }
            }
          }
          valid_read_file_descs[j] = 0;
          i++;
        }
      }
      // Response will be received from ALL monitors. At most one will have a valid answer.
      // In any case, we must reset the set of file_descriptors we expect 'traffic' from.     
      max=0;
      FD_ZERO(&rd);
      for(int k = 0; k < numMonitors; k++){
        if(valid_read_file_descs[k] == 1){
          FD_SET(sock_ids[k], &rd);
          if(sock_ids[k] > max){
            max = sock_ids[k];
          }
        }
      }
      max++;
    }
  }
  if(citizenFound == 0){
    printf("No citizen with id %s\n", citizenID);
  }
  free(citizenData);
  free(command);
  free(pipeWriteBuffer);
  free(pipeReadBuffer);
  free(valid_read_file_descs);
  free(virus);
  free(answer);
  free(ans);
  free(date);
}

void addVaccinationRecords(hashMap *country_map, hashMap *setOfBFs_map, char *inputDir, char *countryName, int *sock_ids, int socketBufferSize, int numMonitors, int sizeOfBloom){
  Country *country = (Country *) find_node(country_map, countryName);
  if(country == NULL){
    printf("No such country: %s\n", countryName);
    return;
  }
  // Finding country responsible for countryName
  int index = country->index;
  char *command = calloc(512, sizeof(char));
  sprintf(command, "newVaccs %s/%s", inputDir, countryName);
  char *pipeWriteBuffer = malloc(socketBufferSize*sizeof(char));
  write_content(command, &pipeWriteBuffer, sock_ids[index], socketBufferSize);
  // Now waiting to receive updated bloom filters from child.
  receiveBloomFiltersFromChild(setOfBFs_map, sock_ids[index], index, socketBufferSize, numMonitors, sizeOfBloom);
  printf("Ready to accept more requests.\n");
  free(command); free(pipeWriteBuffer);
}

void travelStats(hashMap *virusRequest_map, char *virusName, char *date1, char *date2, char *countryTo, int mode){
  virusRequest *vr = (virusRequest *) find_node(virusRequest_map, virusName);
  requests reqs = {0, 0, 0};
  if(vr == NULL){
    printf("No such virus: %s. Try again.\n", virusName);
  }else{
    switch(mode){
      case 0: gatherStatistics(vr, date1, date2, NULL, mode, &reqs);
              break;
      case 1: gatherStatistics(vr, date1, date2, countryTo, mode, &reqs);
              break;
      default:printf("Invalid function mode.\n");
              break;
    }
    printf("TOTAL REQUESTS: %d\n", reqs.total);
    printf("ACCEPTED: %d\n", reqs.accepted);
    printf("REJECTED: %d\n", reqs.rejected);
  }
}