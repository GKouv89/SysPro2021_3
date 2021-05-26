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
    int err = bind(sock_id, serverptr, sizeof(server));
    if(err == -1){
        printf("PORT: %d\n", port);
        perror("bind");
        close(sock_id);
        exit(1);
    }
    // int sock_id = socket(AF_INET, SOCK_STREAM, 0);
    // if(sock_id == -1){
    //     perror("socket call");
    //     close(sock_id);
    //     exit(1);
    // }
    // struct sockaddr_in server, client;
    // struct sockaddr *serverptr = (struct sockaddr *) &server;
    // server.sin_family = AF_INET;
    // server.sin_port = htons(port);
    // memcpy(&(server.sin_addr), localAddress->h_addr, localAddress->h_length);
    // socklen_t serverSize = sizeof(server); 
    // socklen_t clientSize = sizeof(client);
    // err = bind(sock_id, serverptr, serverSize);
    // if(err == -1){
    //     perror("bind failed");
    //     close(sock_id);
    //     exit(1);
    // }
    // // Listening to parent...
    // if(listen(sock_id, 5) < 0){
    //     perror("listen");
    //     close(sock_id);
    //     exit(1);
    // }
    // printf("Everything is great and i'm about to accept.\n");
    // int newsock_id = accept(sock_id, (struct sockaddr *) &client, &clientSize);
    // if(newsock_id == -1){
    //     perror("accept");
    //     close(newsock_id);
    //     close(sock_id);
    //     exit(1);
    // }
    // close(newsock_id); close(sock_id);
    close(sock_id);
    return 0;
}