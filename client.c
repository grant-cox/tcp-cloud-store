#include "proj4.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv) {
    if(argc != 4) {
        fprintf(stderr,"ERROR: Usage: mycloudclient ServerName TCPport SecretKey\n");
        return 1;
    }

    int clientfd = -1, SecretKey, my_argc, dispatch_ret;
	char *host, *port, buf[MAXLINE], *my_argv[128];
    rio_t rio;
    host = argv[1];
    port = argv[2];
    SecretKey = atoi(argv[3]);
    
    clientfd = Open_clientfd(host, port);
    if(clientfd < 0) {
        fprintf(stderr, "ERROR: Unable to locate host %s on %s\n", argv[1], argv[2]);
        return 1;
    }

    if(send_hello_request(clientfd, SecretKey)) {
        fprintf(stderr, "ERROR: Incorrect secret key for server\n");
        return 1;
    }
    printf(">");
    
    //main loop
    while(Fgets(buf, MAXLINE, stdin) != NULL) {
        my_argc = parseline(buf, my_argv);
        dispatch_ret = dispatcher_client(clientfd, my_argv, my_argc);
        if(dispatch_ret == DISPATCH_ERROR || dispatch_ret == DISPATCH_QUIT)
            break;
        printf(">");
    }
    Close(clientfd);
    return 0;
}