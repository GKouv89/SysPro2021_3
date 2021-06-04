#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../include/inputparsing.h"
#include "../include/hashmap.h"
#include "../include/country.h"
#include "../include/citizen.h"
#include "../include/virus.h"

void inputFileParsing(hashMap *countries, hashMap *citizens, hashMap *viruses, FILE *input, int bloomFilterSize, pthread_mutex_t *dataStructAccs){
  size_t line_size = 1024, bytes_read;
  char *line = malloc(line_size*sizeof(char)), *rest;
  char *id, *firstName, *lastName, *country_name, *age, *virus_name, *vacStatus, *date, *temp/* , *dupeVaccinationDate */;
  int found, erroneousRecord;
  Country *country;
  Virus *virus;
  Citizen *citizen, *citizen_compare;
  listNode *possibleDupe;
  while(!feof(input)){
    erroneousRecord = 0;
    bytes_read = getline(&line, &line_size, input);
    if(bytes_read == -1){
      break;
    }
    id = strtok_r(line, " ", &rest);
    firstName = strtok_r(NULL, " ", &rest);
    lastName = strtok_r(NULL, " ", &rest);
    country_name = strtok_r(NULL, " ", &rest);
    age = strtok_r(NULL, " ", &rest);
    virus_name = strtok_r(NULL, " ", &rest);
    vacStatus = strtok_r(NULL, " ", &rest);
    date = strtok_r(NULL, " ", &rest);
    if(strstr(vacStatus, "NO")){
      // normally, we would expect no date after that
      if(date != NULL){
        erroneousRecord = 1;
        printf("ERROR IN RECORD %s %s %s %s %s %s %s %s\n", id, firstName, lastName, country_name, age, virus_name, vacStatus, date);
      }else{
        temp = strtok(vacStatus, "\n");
      }
    }else{
      if(date == NULL){
        erroneousRecord = 1;
        printf("ERROR IN RECORD %s %s %s %s %s %s %s\n", id, firstName, lastName, country_name, age, virus_name, vacStatus);        
      }else{
        temp = strtok(date, "\n");
      }
    }
    if(!erroneousRecord){
      pthread_mutex_lock(dataStructAccs);
      country = (Country *) find_node(countries, country_name);
      if(country == NULL){
        // Index here is -1, as we won't use the index field for lookup purposes
        // in monitorProcess, where this function is called from.
        country = create_country(country_name, -1);
        insert(countries, country_name, country);        
      }
      citizen = (Citizen *) find_node(citizens, id);
      if(citizen == NULL){
        citizen = create_citizen(id, firstName, lastName, atoi(age), country);
        insert(citizens, id, citizen);        
      }else{
        citizen_compare = create_citizen(id, firstName, lastName, atoi(age), country);
        if(!compare_citizens(citizen, citizen_compare)){
          printf("ERROR: CITIZEN %s already exists with different data\n", id);
          destroy_citizen(&citizen_compare);
          continue;
        }
        destroy_citizen(&citizen_compare);
      }
      virus = (Virus *) find_node(viruses, virus_name);
      if(virus == NULL){
        virus = create_virus(virus_name, 7850000000, bloomFilterSize, 16);
        insert(viruses, virus_name, virus);        
      }
      pthread_mutex_unlock(dataStructAccs);
      if(strcmp(vacStatus, "NO") == 0){
        // Assuming there are no duplicates, we only need to add this person to the non vaccinated for list
        pthread_mutex_lock(dataStructAccs);
        insert_in_not_vaccinated_for_list(virus, atoi(id), citizen);
        pthread_mutex_unlock(dataStructAccs);
      }else{
        pthread_mutex_lock(dataStructAccs);
        // Assuming there are no duplicates, we only need to add this person to the bloom filter and vaccinated for list
        insert_in_virus_bloomFilter(virus, id);
        insert_in_vaccinated_for_list(virus, atoi(id), date, citizen);
        pthread_mutex_unlock(dataStructAccs);
      }
    }
  }
  free(line);
}
  
  
  
  
  