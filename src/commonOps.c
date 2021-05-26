#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Finding hostName and host IP address.
struct hostent * findIPaddr(){
    char *hostName = malloc(1024*sizeof(char));
    int err = gethostname(hostName, 1024);
    if(err == -1){
        herror("gethostname in child");
    }
    struct hostent *localAddress = gethostbyname(hostName);
    free(hostName);
    if(localAddress == NULL){
        herror("could not resolve host");
        exit(1);
    }
    return localAddress;
}
