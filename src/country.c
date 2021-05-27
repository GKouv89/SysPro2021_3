#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/country.h"

Country* create_country(const char *name, int index){
  Country *c = malloc(sizeof(Country));
  c->name = malloc((strlen(name) + 1)*sizeof(char));
  strcpy(c->name, name);
  c->index = index;
  c->maxFile = 0;
  return c;
}

int isEqual_country(Country *c, unsigned char *str){
  if(strcmp(c->name, str) == 0){
    return 1;
  }else{
    return 0;
  }
}

// Used to increment the counter of files read in
// a specific country's directory.

void readCountryFile(Country *c){
  c->maxFile++;
}

void destroy_country(Country **c){
  free((*c)->name);
  (*c)->name = NULL;
  free(*c);
  *c = NULL;
}