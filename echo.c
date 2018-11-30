/*
 * echo - read and echo text lines until client closes connection
 */
/* $begin echo */
#include "csapp.h"

void echo(int connfd) 
{
	struct {
	   int what_to_do;
	   int x;
	   int y;
	   int z;
	} data; 
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;
	int size;
	char filename[40];
	FILE *sfp;

   while((n = Rio_readn(connfd, &size, 4)) != 0) { //line:netp:echo:eof
		Rio_readn(connfd, filename, 40);
		sfp = fopen(filename, "w+");
		if (sfp == NULL) printf("Cannot open %s\n", filename);
		else {
		   Rio_readn(connfd, buf, size);
		   fwrite(buf, 1, size, sfp);
		   fclose(sfp);
		}     
	}
}
/* $end echo */

