#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/virus.h"
#include "../include/hashmap.h"
#include "../include/requests.h"
#include "../include/readWriteOps.h"


void checkSkiplist(hashMap *virus_map, char *citizenID, char *virusName, int bufferSize, int sock_id, requests *reqs){
  Virus *virus = (Virus *) find_node(virus_map, virusName);
  listNode *check_for_vacc = lookup_in_virus_vaccinated_for_list(virus, atoi(citizenID));
  char *answer = malloc(255*sizeof(char));
  if(check_for_vacc != NULL){
    sprintf(answer, "YES %s", check_for_vacc->vaccinationDate);
  }else{
    sprintf(answer, "NO");
    reqs->rejected++;
    reqs->total++;
  }
  unsigned int answerLength = strlen(answer);
  char *readPipeBuffer = malloc(bufferSize*sizeof(char));
  char *writePipeBuffer = malloc(bufferSize*sizeof(char));
  write_content(answer, &writePipeBuffer, sock_id, bufferSize);
  while(read(sock_id, readPipeBuffer, sizeof(char)) < 0);
  if(answerLength != 2){
    // in case we sent a vaccination date to the parent process,
    // that process will check whether the travel date
    // is withing six months of the vaccination date and if not,
    // it will reject the request, and it must also let the 
    // child know of that.
    memset(answer, 0, 255*sizeof(char));
    read_content(&answer, &readPipeBuffer, sock_id, bufferSize);
    switch(strcmp(answer, "REJECT")){
      case 0: reqs->rejected++;
            break;
      default: reqs->accepted++;
            break;
    }
    reqs->total++; 
  }
  free(readPipeBuffer);
  free(answer);
  free(writePipeBuffer);
}