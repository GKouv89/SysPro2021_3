#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/readWriteOps.h"
#include "../include/hashmap.h"
#include "../include/setofbfs.h"

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