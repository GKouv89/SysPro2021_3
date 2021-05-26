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
    fprintf(logfile, "Received: port no. = %d\tnumThreads = %d\tsocketBufferSize = %d\tcyclicBufferSize = %d\tsizeofBloom = %d\n", port, numThreads, socketBufferSize, cyclicBufferSize, sizeOfBloom);
    fprintf(logfile, "Received paths:\n");
    if(argc > 11){
        // This check is necessary because perhaps a few monitors do not have folders to handle
        // because there are more monitors than children
        for(int i = 11; i < argc; i++){
            fprintf(logfile, "%s\n", argv[i]);
        }
    }
    struct hostent *localAddress = gethostbyname("localhost");
    if(localAddress == NULL){
        fprintf(logfile, "Could not resolve host\n");
    }else{
        struct in_addr **addr_list = (struct in_addr **) localAddress->h_addr_list;
        for(int i = 0; addr_list[i] != NULL; i++){
            fprintf(logfile, "One local host address is: %s\n", inet_ntoa(*addr_list[i]));
        }
    }
    assert(fclose(logfile) == 0);
    free(logFileName);
    return 0;
}