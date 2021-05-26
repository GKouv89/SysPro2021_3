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
    // WIP: for now, all children take the same port number, 420.
    constArgs[2] = malloc(4*sizeof(char));
    strcpy(constArgs[2], "420");
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
    for(i = 0; i < numMonitors; i++){
        legitimateFolders = 0;
        for(int j = 0; j < subdirCount; j++){
            if(strcmp(alphabeticOrder[j]->d_name, ".") == 0 || strcmp(alphabeticOrder[j]->d_name, "..") == 0){
                continue;
            }
            if(legitimateFolders % numMonitors == i){
                memset(countryPaths[countriesLength], 0, 512*sizeof(char));
                strcpy(countryPaths[countriesLength], input_dir);
                strcat(countryPaths[countriesLength], "/");
                strcat(countryPaths[countriesLength], alphabeticOrder[j]->d_name);
                strcat(countryPaths[countriesLength], "/");
                printf("This is legitimate folder no. %d\n", legitimateFolders);
                printf("alphabeticOrder[%d]->d_name = %s\n", j, alphabeticOrder[j]->d_name);
                printf("Monitor no %d will receive country %s\n", i, countryPaths[countriesLength]);
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
        // Freeing countries.
        for(int j = 11; j < countriesLength; j++){
            free(argArray[j]);
        }
        countriesLength = 0;
    }
    for(i = 0; i < 11; i++){
        free(argArray[i]);
    }
    free(argArray);
    for(i = 0; i < subdirCount; i++){
        free(alphabeticOrder[i]);
    }
    free(alphabeticOrder);
    char *hostName = malloc(1024*sizeof(char));
    if(!gethostname(hostName, 1024)){
        printf("Getting host name in parent results in error\n");
    }else{
        printf("Host, as given from gethostname to parent, is: %s\n", hostName);
    }
    struct hostent *localAddress = gethostbyname(hostName);
    free(hostName);
    if(localAddress == NULL){
        printf("Parent could not resolve host\n");
    }else{
        struct in_addr **addr_list = (struct in_addr **) localAddress->h_addr_list;
        for(int i = 0; addr_list[i] != NULL; i++){
            printf("One local host address as seen from parent is: %s\n", inet_ntoa(*addr_list[i]));
        }
    }
    for(i = 0; i < numMonitors; i++){
        if(wait(NULL) == -1){
            perror("wait");
            exit(1);
        }
    }
    free(children_pids);
    for(i = 0; i < 11; i++){
        free(constArgs[i]);
    }
    for(i = 0; i < countriesCapacity; i++){
        free(countryPaths[i]);
    }
    free(constArgs);
    free(countryPaths);
    free(input_dir);
}