#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/commonOps.h"
#include "../include/readWriteOps.h"
#include "../include/inputparsing.h"
#include "../include/hashmap.h"
#include "../include/country.h"

// extern int errno;

int main(int argc, char *argv[]){
	int i;
    int port = atoi(argv[2]);
    int numThreads = atoi(argv[4]);
    int socketBufferSize = atoi(argv[6]);
    int cyclicBufferSize = atoi(argv[8]);
    int sizeOfBloom = atoi(argv[10]);
    pid_t mypid = getpid();
    char *logFileName = malloc(20*sizeof(char));
    sprintf(logFileName, "log_file.%d", mypid);
	FILE *logfile = fopen(logFileName, "w");
    fprintf(logfile, "Port number I received as a child: %d\n", atoi(argv[2]));
    assert(fclose(logfile) == 0);
    free(logFileName);

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
    printf("Accepted, about to write...\n");
    char *info_buffer = malloc(512*sizeof(char));
    char *writeSockBuffer = malloc(socketBufferSize*sizeof(char));
    char *readSockBuffer = malloc(socketBufferSize*sizeof(char));
    
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

	// Read files from subdirectories, create data structures. 
	char *full_file_name = malloc(512*sizeof(char));
	struct dirent *curr_subdir;
	FILE *curr_file;
	Country *country;
	for(i = 11; i < argc; i++){
		DIR *work_dir = opendir(argv[i]);
		curr_subdir = readdir(work_dir);
		country = create_country(argv[i], -1);
		insert(country_map, argv[i], country);
		while(curr_subdir != NULL){
			if(strcmp(curr_subdir->d_name, ".") == 0 || strcmp(curr_subdir->d_name, "..") == 0){
				curr_subdir = readdir(work_dir);
				continue;
			}
			strcpy(full_file_name, argv[i]);
			strcat(full_file_name, "/");
			strcat(full_file_name, curr_subdir->d_name);
			curr_file = fopen(full_file_name, "r");
			assert(curr_file != NULL);
			inputFileParsing(country_map, citizen_map, virus_map, curr_file, sizeOfBloom);
			readCountryFile(country);
			assert(fclose(curr_file) == 0);			
			curr_subdir = readdir(work_dir);
		}
		closedir(work_dir);
	}
    printf("Bloom filters calculated\n");
  	send_bloomFilters(virus_map, newsock_id, socketBufferSize);
    destroy_map(&country_map); destroy_map(&virus_map); destroy_map(&citizen_map);
    free(info_buffer); free(writeSockBuffer); free(readSockBuffer);
    free(full_file_name);
    close(sock_id); close(newsock_id);
    return 0;
}