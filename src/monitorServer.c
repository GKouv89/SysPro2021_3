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
    strcpy(info_buffer, "Ethan from Maneskin is cute 'cause I said so.");
    char *writeSockBuffer = malloc(socketBufferSize*sizeof(char));
    write_content(info_buffer, &writeSockBuffer, newsock_id, socketBufferSize);
    free(info_buffer); free(writeSockBuffer);
    close(sock_id); close(newsock_id);
    return 0;
}