#ifndef PROJ4_H
#define PROJ4_H

#include "csapp.h"
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>

#define FNAME_MAX_LEN   40    //names no longer than 39 char
#define FILE_MAX_LEN    100000 //100KB

#define HELLO       1
#define STORE       2
#define RETRIEVE    3
#define COPY        4
#define LIST        5
#define DELETE      6

#define DISPATCH_QUIT 1
#define DISPATCH_ERROR 2

#define SERVER_SUCCESS  0
#define SERVER_ERROR   -1



int dispatcher_client(int clientfd, char ** argv, int argc);
int copy_handler_client(int clientfd, char ** argv, int argc);

int response_check_client(int clientfd);

void send_seqno_and_request_client(int clientfd, int request);

//client send functions
int send_hello_request(int clientfd, int key);
int send_store_request(int clientfd, char * src, char * desti);
int send_retrieve_request(int clientfd, char * src, char * desti);
int send_copy_request(int clientfd, char * source, char * desti);
int send_delete_request(int clientfd, char * fname);
int send_list_request(int clientfd);

//server send functions
int serverloop(int connfd, int SecretKey);
int send_seq_and_response(int connfd, int seq, int resp);
int send_store_response(int connfd, int seq);
int send_retrieve_response(int connfd, int seq);
int send_copy_response(int connfd, int seq);
int send_delete_response(int connfd, int seq);
int send_list_response(int connfd, int seq);

int server_file_failure(int connfd, int seq);
int server_malloc_failure(int connfd, int seq, int n_bytes);
int server_write_failure(int connfd, int seq, int n_bytes, FILE * fd);


int parseline(char *buf, char **argv);
#endif