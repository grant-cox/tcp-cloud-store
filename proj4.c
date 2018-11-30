#include "proj4.h"

static int seqno = 0;

/****************************************************************
 ****************************************************************
 ****************************************************************
                            CLIENT SIDE
 ****************************************************************
 ****************************************************************
 ***************************************************************/


//send all the data where it needs to go
int dispatcher_client(int clientfd, char ** argv, int argc) {
    if(argc == 0)
        return 0;
    else if(strcmp(argv[0], "cp") == 0)
        copy_handler_client(clientfd, argv, argc);
    else if(strcmp(argv[0], "rm") == 0) 
        send_delete_request(clientfd, argv[1]);        
    else if(strcmp(argv[0], "list") == 0)
        send_list_request(clientfd);
    else if(strcmp(argv[0], "quit") == 0) 
        return DISPATCH_QUIT;
    else {
        fprintf(stderr, "ERROR: Invalid command.\n");
        return 0;
    }
    return 0;
};

//check each case for which cp is called (there are 4)
int copy_handler_client(int clientfd, char ** argv, int argc) {
    if(argc != 3) {
        fprintf(stderr, "ERROR: Must provide 3 arguments to cp\n");
        return 1;
    }
    char * src = argv[1], * desti = argv[2];
    int src_len = strlen(src), desti_len = strlen(desti);

    if(src_len >= 2) {
        if(src[0] == 'c' && src[1] == ':') {
            src++; src++; //remove c:
            if(desti_len >= 2) {
                if(desti[0] == 'c' && desti[1] == ':') {
                    desti++; desti++; //remove c:
                    // printf("c:file c:file case\n");
                    return send_copy_request(clientfd, src, desti);
                }
                // printf("c:file file case");
                return send_retrieve_request(clientfd, src, desti);
            }
            // printf("c:file file case\n");
            return send_retrieve_request(clientfd, src, desti);
        }
    }
    
    if (desti_len >= 2)
        if(desti[0] == 'c' && desti[1] == ':') {
            desti++; desti++; //remove c:
            // printf("file c:file case\n");
            return send_store_request(clientfd, src, desti);
        }
    

    // printf("file file case: open files\n");
    FILE * file_src = fopen(src, "r"), * file_desti = fopen(desti, "w+");
    if(file_src == NULL || file_desti == NULL) {
        fprintf(stderr, "ERROR: Could not open files for storage\n");
        return 1;
    }
    char filedata[FILE_MAX_LEN];
    int bytes_read = fread(filedata, 1, FILE_MAX_LEN, file_src);
    int bytes_written = fwrite(filedata, 1, bytes_read, file_desti);
    if(bytes_read != bytes_written) {
        fprintf(stderr, "ERROR: bytes read != bytes written\n");
        return 1;
    }
    fclose(file_src); fclose(file_desti);
    return 0;
};

//check the general response from the server --- success, error, seqno
int response_check_client(int clientfd) {
    int seq_check, status;
    Rio_readn(clientfd, &seq_check, 4);    //read seqno 
    Rio_readn(clientfd, &status, 4);       //read status from server
    if(seq_check != seqno)
        fprintf(stderr, "ERROR: seqno reply differs from sent\n");
    else if (status)
        fprintf(stderr, "ERROR: request failed at server\n");
    return 0;
};

//request hello connection from the server
int send_hello_request(int clientfd, int key) {
    send_seqno_and_request_client(clientfd, HELLO);
    Rio_writen(clientfd, &key, 4);
    return response_check_client(clientfd);
};

//send the seqno and the request type from the client side
void send_seqno_and_request_client(int clientfd, int request) {
    seqno++;
    Rio_writen(clientfd, &(seqno), 4);
    Rio_writen(clientfd, &(request), 4);
};


//cp file c:file case --- send a store request to the server
int send_store_request(int clientfd, char * src, char * desti) {    
    send_seqno_and_request_client(clientfd, STORE);

    FILE * fp = fopen(src, "r");
    if(fp == NULL) {
        fprintf(stderr, "ERROR: Could not open %s\n", src);
        return 1;
    }
    char * filecontent[FILE_MAX_LEN];
    int n_bytes = fread(filecontent, 1, FILE_MAX_LEN, fp); //read
    Rio_writen(clientfd, desti, FNAME_MAX_LEN); //write filename
    Rio_writen(clientfd, &n_bytes, 4); //write bytes in file
    Rio_writen(clientfd, filecontent, n_bytes);

    //check response
    return response_check_client(clientfd);
};

//cp c:file file case --- send a retrieve request to the server 
int send_retrieve_request(int clientfd, char * src, char * desti) {
    send_seqno_and_request_client(clientfd, RETRIEVE);
    Rio_writen(clientfd, src, FNAME_MAX_LEN);
    
    //get response
    if(response_check_client(clientfd)) {
        return 1;
    }
    int n_bytes;
    Rio_readn(clientfd, &n_bytes, 4); //get incoming bytes
    FILE * fp = fopen(desti, "w+");
    if(fp == NULL) {
        fprintf(stderr, "ERROR: Could not open %s\n", desti);
        return 1;
    }
    char * filedata = malloc(n_bytes);
    Rio_readn(clientfd, filedata, n_bytes); //get data
    fwrite(filedata, 1, n_bytes, fp);   //write data
    fclose(fp); //close and free
    free(filedata);
    return 0;
};

//cp c: c: case --- copy two files server side 
int send_copy_request(int clientfd, char * source, char * desti) {
    send_seqno_and_request_client(clientfd, COPY);
    Rio_writen(clientfd, source, FNAME_MAX_LEN);
    Rio_writen(clientfd, desti, FNAME_MAX_LEN);

    //get response from server
    if(response_check_client(clientfd))
        return 1;
    return 0;
};

//check for rm c:file and remove appropriately
int send_delete_request(int clientfd, char * fname) { 
    int fname_len = strlen(fname);

    if(fname_len >= 2) { //if length longer than 2, check for c:
        if(fname[0] == 'c' && fname[1] == ':') {
            fname++; fname++; //remove c:
            send_seqno_and_request_client(clientfd, DELETE);
            Rio_writen(clientfd, fname, FNAME_MAX_LEN);
            if(response_check_client(clientfd))
                return 1;
            return 0;
        }
    }
        
    int status = remove(fname);
    if(status) {
        fprintf(stderr, "ERROR: Unable to delete file %s\n", fname);
        return 1;
    }
    return 0;
};

//ask the server to list the files (ls)
int send_list_request(int clientfd) {
    send_seqno_and_request_client(clientfd, LIST);

    //get response
    if(response_check_client(clientfd))
        return 1;
    int n_files;
    Rio_readn(clientfd, &n_files, 4); //get 
    char * list_i = malloc(FNAME_MAX_LEN);
    for(int i = 0; i < n_files; i++) {
        Rio_readn(clientfd, list_i, FNAME_MAX_LEN);
        printf("%s\n", list_i);
    }
    return 0;
};










/****************************************************************
 ****************************************************************
 ****************************************************************
                            SERVER SIDE
 ****************************************************************
 ****************************************************************
 ***************************************************************/

//main loop for the server. wait for hello message from the client.
//loop until client disconnects or error. 
int serverloop(int connfd, int SecretKey) {
    int seq_received, request = 0xFFFFFFFF, key_in, rio_ret = 0;
    while(request != HELLO) { //loop until hello message received
        Rio_readn(connfd, &seq_received, 4);
        Rio_readn(connfd, &request, 4);
    }
    Rio_readn(connfd, &key_in, 4); //get key from client
    if(key_in != SecretKey) {
        fprintf(stderr, "Client at connfd %d failed SecretKey\n", connfd);
        send_seq_and_response(connfd, seq_received, SERVER_ERROR);
        return 1;
    }
    send_seq_and_response(connfd, seq_received, SERVER_SUCCESS);

    while(Rio_readn(connfd, &seq_received, 4)) {
        //rio_ret = Rio_readn(connfd, &seq_received, 4);
        rio_ret = Rio_readn(connfd, &request, 4);
        switch(request) {
            case STORE: { send_store_response(connfd, seq_received);  break;}
            case RETRIEVE: { send_retrieve_response(connfd, seq_received); break;}
            case COPY: { send_copy_response(connfd, seq_received); break;}
            case LIST: { send_list_response(connfd, seq_received); break;}
            case DELETE: { send_delete_response(connfd, seq_received); break;}
            default: { fprintf(stderr, "ERROR: Client sent unknown request\n"); break;}
        };
    }
    return 0;
};

//respond with the most common case, seq and status
int send_seq_and_response(int connfd, int seq, int resp) {
    Rio_writen(connfd, &seq, 4);
    Rio_writen(connfd, &resp, 4);
    return 0;
};

//store file from client into the server
int send_store_response(int connfd, int seq) {
    char fname[FNAME_MAX_LEN];
    char * fdata;
    unsigned int n_bytes;
    Rio_readn(connfd, fname, FNAME_MAX_LEN);
    Rio_readn(connfd, &n_bytes, 4);     //get file size from user
    fdata = malloc(n_bytes);            //pre allocate
    if(fdata == NULL) //on failure, ignore data and return failure
        return server_malloc_failure(connfd, seq, n_bytes);
    
    Rio_readn(connfd, fdata, n_bytes); //read the goods
    
    
    FILE * fd = fopen(fname, "w+"); //open file
    if(fd == NULL)  //on failure, ignore data and return failure
        return server_file_failure(connfd, seqno);    
    
    int n_written = fwrite(fdata, 1, n_bytes, fd); //write to file
    if (n_written != n_bytes)                     //write error?
        return server_write_failure(connfd, seqno, n_bytes, fd);
    
    send_seq_and_response(connfd, seq, SERVER_SUCCESS); //respond success
    fclose(fd);     //close and free
    free(fdata);
    return 0;
};

//send file to the client
int send_retrieve_response(int connfd, int seq) {
    char fname[FNAME_MAX_LEN];
    Rio_readn(connfd, fname, FNAME_MAX_LEN);
    FILE * fd = fopen(fname, "r");          //open
    if(fd == NULL)  //on failure, ignore data and return failure
        return server_file_failure(connfd, seq);

    char fdata[FILE_MAX_LEN];               //read up to 100KB of file
    unsigned int n_bytes = fread(fdata, 1, FILE_MAX_LEN, fd);

    send_seq_and_response(connfd, seq, SERVER_SUCCESS);
    Rio_writen(connfd, &n_bytes, 4);
    Rio_writen(connfd, fdata, n_bytes);
    return 0;
};

//copy a file locally
int send_copy_response(int connfd, int seq) {
    char src[FNAME_MAX_LEN], desti[FNAME_MAX_LEN];
    Rio_readn(connfd, &src, FNAME_MAX_LEN);
    Rio_readn(connfd, &desti, FNAME_MAX_LEN);  //get source file and destinatino

    //open files and check for errors
    FILE * srcfd, * destifd;
    srcfd = fopen(src, "r");
    if(srcfd == NULL) return server_file_failure(connfd, seq);
    destifd = fopen(desti, "w+");
    if(destifd == NULL) return server_file_failure(connfd, seq);

    //copy those bad boys
    char srcdata[FILE_MAX_LEN];
    int bytes_read = fread(srcdata, 1, FILE_MAX_LEN, srcfd);
    char * destidata = malloc(bytes_read);
    int bytes_write = fwrite(srcdata, 1, bytes_read, destifd);
    if(bytes_read != bytes_write) {             //if error
        server_write_failure(connfd, seq, bytes_read, srcfd);
        fclose(destifd);
        return 1;
    }
    
    send_seq_and_response(connfd, seq, SERVER_SUCCESS);
    fclose(srcfd);
    fclose(destifd);
    free(destidata);
    return 0;
};

//client requested to delete server file 
int send_delete_response(int connfd, int seq) {
    char fname[FNAME_MAX_LEN];
    Rio_readn(connfd, fname, FNAME_MAX_LEN);        //get file name
    int status = remove(fname);             //remove
    if(status)      //if error
        return server_file_failure(connfd, seq);
    
    send_seq_and_response(connfd, seq, SERVER_SUCCESS);//success
    return 0;
};

//client invokes "ls" style command 
int send_list_response(int connfd, int seq) {
    struct dirent * entry;          
    DIR * directory = opendir(".");     //open directory for reading 
  
    if (directory == NULL)  {       //if error
        fprintf(stderr, "ERROR: Could not open current directory\n");
        send_seq_and_response(connfd, seq, SERVER_ERROR);
        return 1; 
    } 
  
    char * files[FNAME_MAX_LEN];
    int num_files = 0;
    while ((entry = readdir(directory)) != NULL) {  //get members of directory
        files[num_files] = malloc(FNAME_MAX_LEN);   //allocate as we go along
        strcpy(files[num_files], entry->d_name);
        num_files++;
    }
    send_seq_and_response(connfd, seq, SERVER_SUCCESS);
    Rio_writen(connfd, &num_files, 4);
    for (int i = 0; i < num_files; i++) {   //write members to TCP
        Rio_writen(connfd, files[i], FNAME_MAX_LEN); //write
        free(files[i]);                     //free as we go along
    }
    closedir(directory);        //close
    return 0;
};


///////////////////// SERVER FAILURE NOTIFIERS //////////////////

int server_file_failure(int connfd, int seq) {
    fprintf(stderr, "ERROR: unable to open file\n");
    send_seq_and_response(connfd, seq, SERVER_ERROR);
    return 1;
};
int server_malloc_failure(int connfd, int seq, int n_bytes) {
    fprintf(stderr, "ERROR: malloc failure\n");
    Rio_readn(connfd, NULL, n_bytes ); //on failure, dump data
    send_seq_and_response(connfd, seq, SERVER_ERROR);
    return 1;
};
int server_write_failure(int connfd, int seq, int n_bytes, FILE * fd) {
    fprintf(stderr, "ERROR: bytes written != bytes received\n");
    send_seq_and_response(connfd, seq, SERVER_ERROR);
    fclose(fd);
    return 1;
};



int parseline(char *buf, char **argv) {
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	    return argc;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return argc;
}