#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/commonOps.h"
#include "../include/readWriteOps.h"
#include "../include/hashmap.h"
#include "../include/country.h"
#include "../include/setofbfs.h"
#include "../include/travelMonitorCommands.h"

int main(int argc, char *argv[]){
	if(argc != 13){
		printf("Usage: ./travelMonitor -m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads\n");
		printf("It doesn't matter if the flags are in different order, as long as all are present and the respective argument follows after each flag\n");
		return 1;
	}
	int i;
	int numMonitors, socketBufferSize, sizeOfBloom, cyclicBufferSize, numThreads;
	char *input_dir = malloc(255*sizeof(char));
    // Because we want some of these arguments to be passed to monitorServer,
    // and we'll need them in char form and itoa is not part of the standard,
    // an array with the first 10 (11 if one counts the executable name)
    // is created and updated accordingly here.
    char **constArgs = malloc(11*sizeof(char *));
    constArgs[0] = malloc((strlen("./monitorServer") + 1)*sizeof(char));
    strcpy(constArgs[0], "./monitorServer");
    for(i = 1; i <= 9; i += 2){
        constArgs[i] = malloc(3*sizeof(char));
    }
    strcpy(constArgs[1], "-p");
    strcpy(constArgs[3], "-t");
    strcpy(constArgs[5], "-b");
    strcpy(constArgs[7], "-c");
    strcpy(constArgs[9], "-s");
    for(i = 1; i < 13; i += 2){
		if(strlen(argv[i]) != 2){
			printf("Invalid flag %s\n", argv[i]);
			return 1;
		}
		switch(argv[i][1]){
			case 'm': numMonitors = atoi(argv[i+1]);
					break;
			case 'b': socketBufferSize = atoi(argv[i+1]);
                    constArgs[6] = malloc((strlen(argv[i+1]) + 1)*sizeof(char));
                    strcpy(constArgs[6], argv[i+1]);
                    break;
            case 'c': cyclicBufferSize = atoi(argv[i+1]);
                    constArgs[8] = malloc((strlen(argv[i+1]) + 1)*sizeof(char));
                    strcpy(constArgs[8], argv[i+1]);
                    break;
			case 's': sizeOfBloom = atoi(argv[i+1]);
                    constArgs[10] = malloc((strlen(argv[i+1]) + 1)*sizeof(char));
                    strcpy(constArgs[10], argv[i+1]);
					break;
			case 'i': strcpy(input_dir, argv[i+1]);
					break;
            case 't': numThreads = atoi(argv[i+1]);
                    constArgs[4] = malloc((strlen(argv[i+1]) + 1)*sizeof(char));
                    strcpy(constArgs[4], argv[i+1]);
                    break;
        	default: printf("Invalid flag %s\n", argv[i]);
					return 1;
		}    
	}
    // All children take the same port number, 0.
    // This is so the port can be autoselected.
    constArgs[2] = malloc(6*sizeof(char));
    int nextPortAvailable = 7777;
    sprintf(constArgs[2], "%d", nextPortAvailable);
    // Distributing country names to monitors.
    struct dirent **alphabeticOrder;
	int subdirCount;
	subdirCount = scandir(input_dir, &alphabeticOrder, NULL, alphasort);
	if(subdirCount == -1){
		perror("scandir");
	}
    // Making array of arguments.
    // Initially, will have space for the constant number of arguments, plus 10 country paths
    // If necessary, we realloc.
    char **countryPaths = malloc(10*sizeof(char *));
    int countriesCapacity = 10;
    int countriesLength = 0;
    char **argArray = malloc((12 + countriesLength)*sizeof(char *));
    pid_t pid;
  	pid_t *children_pids = malloc(numMonitors*sizeof(int));
    for(int j = 0; j < 11; j++){
        argArray[j] = malloc((strlen(constArgs[j]) + 1)*sizeof(char));
        strcpy(argArray[j], constArgs[j]);
    }
    for(i = 0; i < 10; i++){
        countryPaths[i] = malloc(512*sizeof(char));
    }
    int legitimateFolders;
    int *sock_ids = malloc(numMonitors*sizeof(int));
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct hostent *localAddress = findIPaddr();
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, localAddress->h_addr, localAddress->h_length);
    // Creating lookup table for countries.
	// Each virus will correspond to  multiple bloomfilters and we must know which one
	// is about the country in question. Each country will hold a payload, the result
	// of the operation (index in alphabeticOrder) % numMonitors + 1. This is the
	// number of the bloom filter in question.
	hashMap *country_map;
	create_map(&country_map, 43, Country_List);
	int payload;
    for(i = 0; i < numMonitors; i++){
        legitimateFolders = 0;
        for(int j = 0; j < subdirCount; j++){
            if(strcmp(alphabeticOrder[j]->d_name, ".") == 0 || strcmp(alphabeticOrder[j]->d_name, "..") == 0){
                continue;
            }
            if(legitimateFolders % numMonitors == i){
                payload = j;
                insert(country_map, alphabeticOrder[j]->d_name, (Country *) create_country(alphabeticOrder[j]->d_name, payload)); 
                memset(countryPaths[countriesLength], 0, 512*sizeof(char));
                strcpy(countryPaths[countriesLength], input_dir);
                strcat(countryPaths[countriesLength], "/");
                strcat(countryPaths[countriesLength], alphabeticOrder[j]->d_name);
                strcat(countryPaths[countriesLength], "/");
                countriesLength++;
                if(countriesLength == countriesCapacity){
                    countriesCapacity *= 2;
                    char **temp = realloc(countryPaths, countriesCapacity*sizeof(char *));
                    assert(temp != NULL);
                    countryPaths = temp;
                    for(int k = countriesLength; k < countriesCapacity; k++){
                        countryPaths[k] = malloc(512*sizeof(char));
                    }
                }
                legitimateFolders++;
            }else{
                legitimateFolders++;
                continue;
            }
        }
        // Preparing argument array for new child.
        char **temp = realloc(argArray, (12 + countriesLength)*sizeof(char *));
        assert(temp != NULL);
        argArray = temp;
        for(int j = 0; j < countriesLength; j++){
            argArray[j + 11] = malloc((strlen(countryPaths[j]) + 1)*sizeof(char));
            strcpy(argArray[j + 11], countryPaths[j]);
        }
        argArray[11 + countriesLength] = NULL;
        if((pid = fork()) < 0){
            perror("fork");
            return 1;
        }
        if(pid == 0){
            if(execv(argArray[0], argArray) < 0){
                perror("execv");
                return 1;
            }
        }else{
            children_pids[i] = pid;
        }
        if((sock_ids[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket creation in parent");
            exit(1);
        }
        server.sin_port = htons(nextPortAvailable);
        /* Initiate connection */
        while(connect(sock_ids[i], serverptr, sizeof(server)) < 0);
        printf("Connected to child no: %d\n", i);
        // Freeing countries.
        for(int j = 0; j <= countriesLength; j++){
            free(argArray[j + 11]);
        }
        temp = realloc(argArray, 11*sizeof(char *));
        assert(temp != NULL);
        argArray = temp;
        nextPortAvailable++;
        sprintf(argArray[2], "%d", nextPortAvailable);
        countriesLength = 0;
    }
    char *info_buffer = calloc(512, sizeof(char));
    char *readSockBuffer = malloc(socketBufferSize*sizeof(char));
    char *writeSockBuffer = malloc(socketBufferSize*sizeof(char));
    
    // Creating hashMap of sets of bloomfilters.
	hashMap *setOfBFs_map;
	create_map(&setOfBFs_map, 3, BFLookup_List);
	setofbloomfilters *curr_set;
	// This array shows which monitors we have YET to receive
	// bloomfilters from. When we receive the bloom filters of one monitor,
	// the respective element in this array takes the value zero.
	int *read_bloom_descs = malloc(numMonitors*sizeof(int));
    fd_set rd;
	FD_ZERO(&rd);
    int max = 0;
	for(i = 0; i < numMonitors; i++){
		FD_SET(sock_ids[i], &rd);
		read_bloom_descs[i] = 1;
        if(sock_ids[i] > max){
            max = sock_ids[i];
        }
    }
    max++;
	char *virusName = calloc(255, sizeof(char));
	for(i = 0; i < numMonitors; ){
		if(select(max, &rd, NULL, NULL, NULL) == -1){
			perror("select virus names");
		}else{
			// Perhaps more than one file descriptors are ready to send data to parent,
			// in the case two monitors have roughly the same amount of work to do.
			for(int k = 0; k < numMonitors; k++){
				if(read_bloom_descs[k] == 1 && FD_ISSET(sock_ids[k], &rd)){
					// We will now read from the monitor process no. k
					receiveBloomFiltersFromChild(setOfBFs_map, sock_ids[k], k, socketBufferSize, numMonitors, sizeOfBloom);
					// Increasing number of children we have received filters from
                    printf("Received bloom filters from child no %d.\n", k);
					i++;
					// We don't expect any more 'traffic' from this child.
					read_bloom_descs[k] = 0;
				}
			}
			// After reading from all children that were ready in this rep, we must reinitialize the set of file descs
			// that we expect 'traffic' from.
			max = 0;
			FD_ZERO(&rd);
			for(int j = 0; j < numMonitors; j++){
				if(read_bloom_descs[j] == 1){
					FD_SET(sock_ids[j], &rd);
					if(sock_ids[j] > max){
						max = sock_ids[j];
					}
				}
			}
			max++;			
		}
	}
	free(read_bloom_descs);
	destroy_map(&setOfBFs_map);    
    
    free(info_buffer);
    free(readSockBuffer);
    free(writeSockBuffer);
    for(i = 0; i < numMonitors; i++){
        if(wait(NULL) == -1){
            perror("wait");
            exit(1);
        }
    }
    for(i = 0; i < numMonitors; i++){
        close(sock_ids[i]);
    }
    for(i = 0; i < 11; i++){
        free(argArray[i]);
    }
    free(argArray);
    for(i = 0; i < subdirCount; i++){
        free(alphabeticOrder[i]);
    }
    free(alphabeticOrder);
    free(children_pids);
    for(i = 0; i < 11; i++){
        free(constArgs[i]);
    }
    for(i = 0; i < countriesCapacity; i++){
        free(countryPaths[i]);
    }
    destroy_map(&country_map);
    free(virusName);
    free(constArgs);
    free(countryPaths);
    free(input_dir);
    free(sock_ids);
}