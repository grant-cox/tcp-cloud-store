#include "proj4.h"
#include <stdlib.h>


int main(int argc, char ** argv) {
    if(argc != 3) {
        fprintf(stderr,"ERROR: Usage: mycloudclient TCPport SecretKey\n");
        exit(0);
    }

    int listenfd, connfd, SecretKey;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */
    char client_hostname[MAXLINE], client_port[MAXLINE], * port;

    port = argv[1];
    SecretKey = atoi(argv[2]);

    listenfd = Open_listenfd(port);
    printf("opened on port %s\n", port);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage); 
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        serverloop(connfd, SecretKey);
        Close(connfd);
    }
    return 0;
}
