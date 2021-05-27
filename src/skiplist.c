#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/skiplist.h"
#include "../include/dateOps.h"
#include "../include/country.h"

void create_skiplist(skipList **s, unsigned long long exp_data_count){
  (*s) = malloc(sizeof(skipList));
  (*s)->height = 1; 
  (*s)->max_height = log2(exp_data_count);
  (*s)->levels = malloc(sizeof(*((*s)->levels)));
  create_list(&((*s)->levels[0]));
}

char* insert_skipnode(skipList *s, int id, char *vacDate, Citizen *c){
  listNode **startingNodes = malloc(s->height*sizeof(listNode *));
  // As we are about to descend into the skiplist's levels,
  // we take note of the point in the list where the value of id
  // became greater than the value of the list node, and therefore where
  // we will move to the next, lower, level.
  // This way, when we decide in how many levels the current id will
  // be inserted into, we will know for each level after which node
  // we should insert the new one.
  int error = 0;
  char *dupeVaccinationDate = search_skip(s, id, startingNodes, &error);
  if(error == 1){
    // ID already exists in skip list
    free(startingNodes);
    return dupeVaccinationDate;
  }
  int idHeight = 1;
  while(rand() % 1000 < 1000*p && idHeight < s->max_height){
    idHeight++;
  }
  if(idHeight > s->height){
    // The skiplist grew, new level(s) will be created.
    list **temp = realloc(s->levels, idHeight*sizeof(*(s->levels)));
    assert(temp != NULL);
    s->levels = temp;
    // Since new level(s) will be added, we also take note of where
    // the new node will be inserted into these lists: NULL means at the
    // very start.
    listNode **temp2 = realloc(startingNodes, idHeight*sizeof(listNode *));
    assert(temp2 != NULL);
    startingNodes = temp2;
    for(int i = s->height; i < idHeight; i++){
      create_list(&(s->levels[i]));
      startingNodes[i] = NULL; // these new lists are empty
      // so no boundary exists for the insertion
    }
    s->height = idHeight;
  }
  listNode *prevConnection = NULL; 
  // this will connect the node of a list
  // with the corresponding one in the list right above that
  listNode *currConnection;
  
  for(int i = 0; i < s->height ;i++){
    if(i == 0){
      currConnection = insert_node(s->levels[i], startingNodes[i], id, vacDate, c);
    }else{
      currConnection = insert_node(s->levels[i], startingNodes[i], id, NULL, NULL);
    }
    currConnection->bottom = prevConnection;
    prevConnection = currConnection;
    startingNodes[i] = NULL;
  }
  free(startingNodes);
  return NULL;
}

char* search_skip(skipList *s, int id, listNode *startingNodes[], int *error){
  boundaries *bounds_ret = malloc(sizeof(boundaries));
  boundaries *bounds_arg = malloc(sizeof(boundaries));
  // Bounds_arg the nodes between which we will be searching in that specific level
  // For the top level, the entire list is searched.
  // When two nodes whose values are such that first_node_id < id < second_node_id,
  // the search on the next level must take place between the nodes that are 'directly underneath'
  // these two nodes, and so, bounds_ret contains the bottom field of these two nodes.
  // Now, if the id is smaller than the id of the first node in the list/level,
  // then bounds_ret->start will be NULL and bounds_ret->end will be the bottom field of the list's first node.
  // Respectively, if the id is greater than the id of the last node in the list/level,
  // then bounds_ret->start will be the bottom field of the list's last node and bounds_end->end will be NULL,
  // which means in the next level the search will occur from bounds_ret->start and forward.
  bounds_arg->start = s->levels[s->height - 1]->front;
  bounds_arg->end = s->levels[s->height - 1]->rear;
  listNode *futureSN;
  char *dupeVaccinationDate;
  for(int i = s->height - 1; i >= 0; i--){
    dupeVaccinationDate = search(s->levels[i], id, bounds_arg->start, bounds_arg->end, &bounds_ret, error, &futureSN);
    if(*error == 1){ // Element already in skiplist
      startingNodes[i] = NULL;// WIP: what do we insert in startingNodes?
      free(bounds_ret);
      free(bounds_arg);
      return dupeVaccinationDate;
    }
    startingNodes[i] = futureSN; // Keeping track of where the new node will potentially be inserted, if 
    // the node's height reaches the current level.
    // For insertion, for all lists except the bottom one,
    // the startingNode will be the node right after which the new one will be inserted
    bounds_arg->start = bounds_ret->start;
    bounds_arg->end = bounds_ret->end;
  }
  free(bounds_arg);
  free(bounds_ret);
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////
// Search is used to locate where a node will be placed after insertion in a list    //
// Lookup on the other hand is used to locate whether a node is in the skiplist and, //
// if so, provide info on that citizen's vaccination state                           //
///////////////////////////////////////////////////////////////////////////////////////

listNode* lookup_skiplist(skipList *s, int id){
  boundaries *bounds_ret = malloc(sizeof(boundaries));
  boundaries *bounds_arg = malloc(sizeof(boundaries));
  bounds_arg->start = s->levels[s->height - 1]->front;
  bounds_arg->end = s->levels[s->height - 1]->rear;
  listNode *futureSN;
  int found = 0;
  for(int i = s->height - 1; i >= 0; i--){
    search(s->levels[i], id, bounds_arg->start, bounds_arg->end, &bounds_ret, &found, &futureSN);
    // futureSN is ignored here
    if(found == 1){ // Element already in skiplist
      break;
    }
    bounds_arg->start = bounds_ret->start;
    bounds_arg->end = bounds_ret->end;
  }
  listNode *infoNode;
  // if the id was found in one of the list's upper levels (any level that is not the bottom one)
  // the bottom pointers to the bottom list are followed (cascade function), 
  // where the vaccinattionDate is included. 
  if(found){
    infoNode = cascade(bounds_ret->start);
    free(bounds_arg);
    free(bounds_ret);
    return infoNode;
  }else{
    free(bounds_arg);
    free(bounds_ret);
    return NULL;
  }
}

void print_skiplist(skipList *s){
  for(int i = s->height - 1; i >= 0; i--){
    printf("LEVEL: %d\n", i);
    print_list(s->levels[i]);
  }
}


//////////////////////////////////////////////////////////////
// In this function, we start from top to bottom            //
// hoping to locate the node in one of the lists quickly.   //
// Once we do, all we must do is follow the trail of bottom //
// pointers to the lowest level, and delete in constant time//
// in each of the remaining lists.                          //
//////////////////////////////////////////////////////////////

void delete_skipnode(skipList *s, int id){
  int mode = 0;
  listNode *result;
  listNode *location = NULL;
  for(int i = s->height - 1; i >= 0; i--){
    result = delete_node(s->levels[i], mode, id, location);
    if(result != NULL){
      mode = 1;
      location = result;
    }
  }
}


void destroy_skiplist(skipList **s){
  if(*s == NULL){
    return;
  }
  for(int i = 0; i < (*s)->height; i++){
    destroy_list(&((*s)->levels[i]));
  }
  free((*s)->levels);
  (*s)->levels = NULL;
  free(*s);
  (*s) = NULL;
}