#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/bucketlist.h"
#include "../include/country.h"
#include "../include/citizen.h"
#include "../include/virus.h"
#include "../include/setofbfs.h"
#include "../include/virusRequest.h"
#include "../include/readWriteOps.h"

void create_bucketList(bucketList **bl, typeOfList type){
  (*bl) = malloc(sizeof(bucketList));
  (*bl)->front = (*bl)->rear = NULL;
  (*bl)->type = type;
}

void insert_bucketNode(bucketList *bl, void *content){
  bucketNode *new_node = malloc(sizeof(bucketNode));
  new_node->content = content;
  new_node->next = NULL;
  if(bl->front == NULL && bl->rear == NULL){
    bl->front = new_node;
    bl->rear = new_node;
  }else{
    bl->rear->next = new_node;
    bl->rear = new_node;
  }
}

//////////////////////////////////////////////////////////
// The search function called from the find_node        //
// function of the hashmap module. According to the type//
// of bucketlist (and therefore, the type of hashmap)   //
// the appropriate comparison function is called.       //
//////////////////////////////////////////////////////////

void* search_bucketList(bucketList *bl, char *str){
  bucketNode *temp = bl->front;
  while(temp){
    if(bl->type == Country_List){
      if(isEqual_country(temp->content, str)){
        return temp->content;
      }
    }else if(bl->type == Virus_List){
      if(isEqual_virus(temp->content, str)){
        return temp->content;
      }
    }else if(bl->type == Citizen_List){
      if(isEqual_citizen(temp->content, str)){
        return temp->content;
      }
    }else if(bl->type == VirusRequest_List){
      if(isEqual_virusRequest(temp->content, str)){
        return temp->content;
      }
    }else{
      if(isEqual_setOfBFs(temp->content, str)){
        return temp->content;
      }
    }
    temp = temp->next;
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// This is called when the monitor process is about to send the bloomfilters //
// after the first reading of the files.                                     //
///////////////////////////////////////////////////////////////////////////////

void send_virus_Bloomfilters(bucketList *bl, int readfd, int writefd, int bufferSize){
   if(bl->type == Virus_List){
      bucketNode *temp = bl->front;
      unsigned int virusNameLength, charsCopied, charsToWrite;
      char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
      char *pipeReadBuffer = malloc(bufferSize*sizeof(char));
      bloomFilter *bf;
      int bfSize;
      int bytesTransferred;
      while(temp){
        // First, send virus name length.
        virusNameLength = strlen(((Virus *)temp->content)->name);
        if(write(writefd, &virusNameLength, sizeof(int)) < 0){
            perror("write virus name length");
        }else{
          charsCopied = 0;
          while(charsCopied < virusNameLength){
            strncpy(pipeWriteBuffer, ((Virus *)temp->content)->name + charsCopied, bufferSize);
            if(write(writefd, pipeWriteBuffer, bufferSize) < 0){
              perror("write virus name chunk");
            }
            charsCopied += bufferSize;
          }
          // Waiting for confirmation that the virus name was received in its entirety.
          while(read(readfd, pipeReadBuffer, bufferSize) < 0);
          bf = ((Virus *)temp->content)->virusBF;
          bytesTransferred = 0;
          bfSize = (bf->size)/8;
          // Now, since bloom filter size is known, no size is sent from child, just
          // the bufferSize chunks of the filter.
          while(bytesTransferred < bfSize){
            if(bfSize - bytesTransferred < bufferSize){
              charsToWrite = bfSize - bytesTransferred;
            }else{
              charsToWrite = bufferSize;
            }
            memcpy(pipeWriteBuffer, bf->filter + bytesTransferred, charsToWrite*sizeof(char));
            bytesTransferred += charsToWrite;
            if(write(writefd, pipeWriteBuffer, charsToWrite*sizeof(char)) < 0){
              perror("write bf chunk\n");
            }
          }
          while(read(readfd, pipeReadBuffer, bufferSize) < 0);
        }
        temp = temp->next;
      }
      free(pipeWriteBuffer);
      free(pipeReadBuffer);
   } 
}

////////////////////////////////////////////////////////////////////////////////////////////////
// This function is called after one of the children was killed and the parent                //
// spawns a new child in its place. It is checked which country was processed by the old child//
// and those are communicated to the new child.                                               //
////////////////////////////////////////////////////////////////////////////////////////////////

void findCountriesForChild(bucketList *bl, char ***countries, int *countryIndex, int monitorIndex){
  Country *country; 
  if(bl->type == Country_List){
    bucketNode *temp = bl->front;
    while(temp){
      country = (Country *)temp->content; 
      if(country->index == monitorIndex){
        (*countries)[*countryIndex] = malloc((strlen(country->name) + 1)*sizeof(char));
        strcpy((*countries)[*countryIndex], country->name);
        (*countryIndex)++;
      }
      temp = temp->next;
    }
  }
}

// Used in log file creation.

void printSubdirNames(bucketList *bl, FILE *fp){
  if(bl->type == Country_List){
    bucketNode *temp = bl->front;
    while(temp){
      fprintf(fp, "%s\n", ((Country *)temp->content)->name);
      temp = temp->next;
    }
  }
}


////////////////////////////////////////////////////////////////////////
// The command that is called when searchVaccinationStatus is called  //
// without specifying the virus. Called for the list of every         //
// hash bucket, for every node of the list (every virus)              //
// a search through the skiplist containing the vaccinated            //
// people for the virus occurs.                                       //
// An appropriate response is forwarded to the parent.                //
////////////////////////////////////////////////////////////////////////

void vacStatus_all(bucketList *bl, unsigned char *citizenID, int readfd, int writefd, int bufferSize){
  char *pipeWriteBuffer = malloc(bufferSize*sizeof(char));
  char *answer = malloc(255*sizeof(char));
  char confirmation;
  if(bl->type == Virus_List){
    bucketNode *temp = bl->front;
    listNode *res;
    while(temp){
      res = lookup_in_virus_vaccinated_for_list((Virus *)temp->content, atoi(citizenID));
      write_content(((Virus *) temp->content)->name, &pipeWriteBuffer, writefd, bufferSize);
      while(read(readfd, &confirmation, sizeof(char)) < 0);
      if(res == NULL){
        sprintf(answer, "NO");
      }else{
        sprintf(answer, "YES %s", res->vaccinationDate);
      }
      write_content(answer, &pipeWriteBuffer, writefd, bufferSize);
      while(read(readfd, &confirmation, sizeof(char)) < 0);
      temp = temp->next;
    }
  }
  free(pipeWriteBuffer);
  free(answer);
}

void destroy_bucketList(bucketList **bl){
  bucketNode *temp = (*bl)->front;
  bucketNode *to_del;
  while(temp){
    to_del = temp;
    temp = temp->next;
    if((*bl)->type == Country_List){
      Country *to_destroy = (Country *)to_del->content;
      destroy_country(&to_destroy);
    }else if((*bl)->type == Virus_List){
      Virus *to_destroy = (Virus *)to_del->content;
      destroy_virus(&to_destroy);
    }else if((*bl)->type == Citizen_List){
      Citizen *to_destroy = (Citizen *)to_del->content;
      destroy_citizen(&to_destroy);
    }else if((*bl)->type == VirusRequest_List){
      virusRequest *to_destroy = (virusRequest *)to_del->content;
      destroy_virusRequest(&to_destroy);
    }else{
      setofbloomfilters *to_destroy = (setofbloomfilters *)to_del->content;
      destroy_setOfBFs(&to_destroy);
    }
    free(to_del);
    to_del = NULL;
  }
  free(*bl);
  *bl = NULL;
}
