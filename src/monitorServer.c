#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/commonOps.h"
#include "../include/readWriteOps.h"
#include "../include/inputparsing.h"
#include "../include/hashmap.h"
#include "../include/country.h"
#include "../include/monitorServerCommands.h"
#include "../include/cyclicBuffer.h"
#include "../include/requests.h"

extern int errno;

int main(int argc, char *argv[]){
	int i;
    int port = atoi(argv[2]);
    int numThreads = atoi(argv[4]);
    int socketBufferSize = atoi(argv[6]);
    int cyclicBufferSize = atoi(argv[8]);
    int sizeOfBloom = atoi(argv[10]);

    struct hostent *localAddress = findIPaddr();
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, localAddress->h_addr , localAddress->h_length);    
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    server.sin_port = htons(port);
    int sock_id = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_id == -1){
        perror("socket call");
        close(sock_id);
        exit(1);
    }
    int value = 1;
    if(setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, &value,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    int err = bind(sock_id, serverptr, sizeof(server));
    if(err == -1){
        printf("PORT: %d\n", port);
        perror("bind");
        close(sock_id);
        exit(1);
    }
    // Listening to parent...
    if(listen(sock_id, 5) < 0){
        perror("listen");
        close(sock_id);
        exit(1);
    }
    // Accepting connection.
    struct sockaddr_in client;
    struct sockaddr *clientptr = (struct sockaddr *) &client;
    socklen_t clientlen = sizeof(client);
    int newsock_id;
    if((newsock_id = accept(sock_id, clientptr, &clientlen)) < 0){
        perror("accept");
        close(sock_id);
        close(newsock_id);
        exit(1);
    }
    // printf("Accepted, about to write...\n");

    ///////////////////////////////////////////////////////////////
	hashMap *country_map, *virus_map, *citizen_map;
	// Prime bucket of numbers for all hashmaps
	// virusesFile has only about 12 viruses
	// and countriesFile contains 195 countries
	// so the primes were chosen to be an order of magnitude
	// smaller than the size of the respective file
	// for citizens, perhaps the input file will have few lines,
	// but perhaps it will have upwards of 1000 records.
	// In any case, this allows for insertion of more records
	// in case of more input files.
	create_map(&country_map, 43, Country_List);
	create_map(&virus_map, 3, Virus_List);
	create_map(&citizen_map, 101, Citizen_List);
	//////////////////////////////////////////////////////////////// 


    // Testing thread synchronization.
    // After receiving everything in argv, this thread will try to 
    // put the folder paths initially in the buffer, and let the children read from it. Just that.
    int filesProduced = 0, filesConsumed = 0;
    pthread_t *threads = malloc(numThreads*sizeof(pthread_t));

    cyclicBuffer *cB;
    create_cyclicBuffer(&cB, cyclicBufferSize);
    pthread_mutex_t mtx, dataStructAccs, filesDone;
    pthread_cond_t cond_nonempty, cond_nonfull, allfiles_consumed;
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&dataStructAccs, 0);
    pthread_mutex_init(&filesDone, 0);
    pthread_cond_init(&cond_nonfull, 0);
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&allfiles_consumed, 0);

    consumerThreadArgs *args = malloc(numThreads*sizeof(consumerThreadArgs));
    for(int i = 0; i < numThreads; i++){
        args[i].cB = cB;
        args[i].mtx = &mtx;
        args[i].cond_nonfull = &cond_nonfull;
        args[i].cond_nonempty = &cond_nonempty;
        args[i].allfiles_consumed = &allfiles_consumed;
        args[i].dataStructAccs = &dataStructAccs;
        args[i].hasThreadFinished = 0;
        args[i].country_map = country_map;
        args[i].citizen_map = citizen_map;
        args[i].virus_map = virus_map;
        args[i].sizeOfBloom = sizeOfBloom;
        args[i].filesConsumed = &filesConsumed;
    }

    for(int i = 0; i < numThreads; i++){
        pthread_create(&(threads[i]), NULL, consumer, (consumerThreadArgs *) &args[i]);
    }

    producer(argv, cB, &cond_nonempty, &cond_nonfull, &mtx, &filesProduced);
    
    char *info_buffer = malloc(512*sizeof(char));
    char *writeSockBuffer = malloc(socketBufferSize*sizeof(char));
    char *readSockBuffer = malloc(socketBufferSize*sizeof(char));
    
	// Read files from subdirectories, create data structures. 
	char *full_file_name = malloc(512*sizeof(char));
    char *big_folder_name, *little_folder_name, *rest;
    char *argument;
	struct dirent *curr_subdir;
	FILE *curr_file;
	Country *country;
    while(filesConsumed != filesProduced){
        pthread_cond_wait(&allfiles_consumed, &filesDone);
    }
  	send_bloomFilters(virus_map, newsock_id, socketBufferSize);
    // Resetting counters in case of addition of new records.
    filesProduced = 0;
    filesConsumed = 0;

    unsigned int charactersParsed, charactersRead;
    unsigned int commandLength, charactersCopied, charsToWrite;
	char *command = calloc(255, sizeof(char));
	char *command_name;
	char *citizenID = malloc(5*sizeof(char));
	char *virusName = malloc(50*sizeof(char));
	char *countryFrom = malloc(255*sizeof(char));
    char *countryDir = malloc(512*sizeof(char));
    fd_set rd;
    requests reqs = {0, 0, 0};
    while(1){
		FD_ZERO(&rd);
		FD_SET(newsock_id, &rd);
		if(select(newsock_id + 1, &rd, NULL, NULL, NULL) < 1 && errno == EINTR){
			perror("select child while waiting for command");
		}else{
			if(read(newsock_id, &commandLength, sizeof(int)) < 0){
				perror("read request length");
			}else{
                memset(command, 0, 255*sizeof(char));
                charactersParsed = 0;
                while(charactersParsed < commandLength){
                    if((charactersRead = read(newsock_id, readSockBuffer, socketBufferSize)) < 0){
                        continue;
                    }else{
                        strncat(command, readSockBuffer, charactersRead*sizeof(char));
                        charactersParsed += charactersRead;
                    }
                }
                command_name = strtok_r(command, " ", &rest);
                if(strcmp(command_name, "checkSkiplist") == 0){
                    if(sscanf(rest, "%s %s %s", citizenID, virusName, countryFrom) == 3){
                        Country *country = (Country *) find_node(country_map, countryFrom);
                        Citizen *citizen = (Citizen *) find_node(citizen_map, citizenID);
                        // One of three error cases
                        // Citizen ID doesn't exist (at least in this monitor)
                        // Citizen ID exists but countryFrom is invalid and doesn't
                        // Citizen ID exists and so does countryFrom, but it is not the
                        // citizen's actual country!
                        // If the last one wasn't caught, the checks could go through 
                        // the bloom filter/skiplist with no problem.
                        if(country == NULL || citizen == NULL || citizen->country != country){
                            // Send BAD COUNTRY response to parent.
                            char *response = calloc(12, sizeof(char));
                            strcpy(response, "BAD COUNTRY");
                            write_content(response, &writeSockBuffer, newsock_id, socketBufferSize);
                            while(read(newsock_id, readSockBuffer, sizeof(char)) < 0);
                            free(response);
                        }else{
                            checkSkiplist(virus_map, citizenID, virusName, socketBufferSize, newsock_id, &reqs);
                        }
                    }
                }else if(strcmp(command_name, "checkVacc") == 0){
                    if(sscanf(rest, "%s", citizenID) == 1){
                        checkVacc(citizen_map, virus_map, citizenID, newsock_id, socketBufferSize);
                    }
                }else if(strcmp(command_name, "/exit\n") == 0){
                    break;
                }else if(strcmp(command_name, "newVaccs") == 0){
                    if(sscanf(rest, "%s", countryDir) == 1){
                        // countryFrom will be of format: input_dir/countryName
                        // so, we are about to extract the countryName
                        argument = malloc((strlen(countryDir) + 1)*sizeof(char));
                        strcpy(argument, countryDir);
                        big_folder_name = strtok_r(argument, "/", &rest);
                        little_folder_name = strtok_r(NULL, "/", &rest);
                        Country *country = (Country *) find_node(country_map, little_folder_name);
                        addNewRecords(country, countryDir, cB, &cond_nonempty, &cond_nonfull, &mtx, &filesProduced);
                        while(filesConsumed != filesProduced){
                            pthread_cond_wait(&allfiles_consumed, &filesDone);
                        }
                        send_bloomFilters(virus_map, newsock_id, socketBufferSize);
                        filesProduced = 0;
                        filesConsumed = 0;
                        free(argument);
                    }
                }else{
                    printf("Unknown command in child: %s\n", command_name);
                }
			}
		}
	}
    // Write END in cyclicBuffer once for each thread that has been created
    killThreadPool(numThreads, cB, &cond_nonempty, &cond_nonfull, &mtx);
    
    for(int i = 0; i < numThreads; i++){
        pthread_join(threads[i], 0);
    }
    free(threads);
    destroy_cyclicBuffer(&cB);
    free(args);
    pthread_cond_destroy(&cond_nonempty);
    pthread_cond_destroy(&cond_nonfull);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&filesDone);
    pthread_mutex_destroy(&dataStructAccs);
    pthread_cond_destroy(&allfiles_consumed);

    free(command); free(citizenID); free(countryFrom); free(virusName); free(countryDir);
    // Making log file
    pid_t mypid = getpid();
    char *logFileName = malloc(20*sizeof(char));
    sprintf(logFileName, "log_file.%d", mypid);
	FILE *logfile = fopen(logFileName, "w");
    printSubdirectoryNames(country_map, logfile);
    fprintf(logfile, "TOTAL TRAVEL REQUESTS %d\nACCEPTED %d\nREJECTED %d\n", reqs.total, reqs.accepted, reqs.rejected);
    assert(fclose(logfile) == 0);
    free(logFileName);

    destroy_map(&country_map); destroy_map(&virus_map); destroy_map(&citizen_map);
    free(info_buffer); free(writeSockBuffer); free(readSockBuffer);
    free(full_file_name);
    close(sock_id); close(newsock_id);
    return 0;
}