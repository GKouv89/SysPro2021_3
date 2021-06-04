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
    printf("Citizen vaccinated on %s\n", check_for_vacc->vaccinationDate);
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

void checkVacc(hashMap *citizen_map, hashMap *virus_map, char *citizenID, int sock_id, int bufferSize){
  Citizen *citizen = (Citizen *) find_node(citizen_map, citizenID);
  char *citizenData = calloc(1024, sizeof(char));
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  if(citizen == NULL){
    strcpy(citizenData, "NO SUCH CITIZEN");
    write_content(citizenData, &pipeWriteBuffer, sock_id, bufferSize);
    free(pipeWriteBuffer);
    free(citizenData);
    return;
  }
  print_citizen(citizen, &citizenData);
  write_content(citizenData, &pipeWriteBuffer, sock_id, bufferSize);
  char confirmation;
  while(read(sock_id, &confirmation, sizeof(char)) < 0);
  // All viruses in the virus map check whether the citizen with this ID is in their skiplist and notify the parent.
  lookup_vacStatus_all(virus_map, citizenID, sock_id, bufferSize);
  free(citizenData);
  free(pipeWriteBuffer);
}