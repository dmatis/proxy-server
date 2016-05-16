#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include "parse_get.c"
#include <sys/stat.h>


char hashed_file_name[100];

struct sockaddr_in bind_socket(int fam, int addr, int port)
{
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = fam;
    sock_addr.sin_addr.s_addr = addr;
    sock_addr.sin_port = port;
	return sock_addr;
}

long hash(unsigned char *str)
{
    long hash = 5381;
    int c;

    while (c = *str++)
	{
        hash = ((hash << 5) + hash) + c;
	}
    return hash;
}

void hash_file_name(char* file_name)
{
	long l = hash(file_name);
	sprintf(hashed_file_name, "%ld", l);
	strcat(hashed_file_name, ".txt");
}

void* connection_handler(void *);

int main(int argc, char* argv[]) {
	
    //Listening Socket Descriptor
    int socket_desc, *new_sock;
    struct sockaddr_in server, client;
    char *message;

    if (argc != 3) {
        puts("correct usage: ./proxyFilter <PORT> <blacklist_file>\n");
        return 1;
    }

    int port = atoi(argv[1]);
    FILE *file = fopen( argv[2], "r" );
	// Generate blacklisted sites from file
	char contents[12000];
    if (file == 0){
        puts("Could not open the blacklist file");
    } else {
        int x;        
		while ((x = fgetc(file)) != EOF){
			char charx = x;
			char charxlower = tolower(x);
			char* xstr = &charxlower;
			strcat(contents,xstr);
        }
		banned_sites = str_split(contents, '\n');
    }
	fclose(file);
	
    //Socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        puts("Could not create socket");
    }
    // eliminate the "Address already in use" error message
    int yes=1;
    if (setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    } 

	server = bind_socket(AF_INET, INADDR_ANY, htons(port));
    if ( bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
    	puts("bind error");
        return 1;
    }

    //Listen
    listen(socket_desc, 3);
    printf("Waiting for incoming connections...\n");

    //Create 3 worker threads
    pthread_t tid[3];
    int err;
    int i = 0;
    while (i < 3) {
        err = pthread_create(&tid[i], NULL, connection_handler, (void*) &socket_desc);
        i++;
    }
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);
}

void *connection_handler(void *socket_desc) {
    int host_socket, client_socket, c;
    struct sockaddr_in client;

    char *message;
    char buffer [4096];
    char *hostname;
    char ip[100];
    struct hostent *he;
    struct in_addr **addr_list;
  
    //variables needed for parsing incoming requests to server
    char http_header[4096];
    char filename[1024];
	char protocol[10];
    char* strptr;
    int responseStart = 1;
    int badResponse = 0;
    FILE *fp;

    c = sizeof(struct sockaddr_in);
    while( (client_socket = accept( *(int*) socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
        
        puts("Connection accepted");
        
        //Set client socket timer
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

        int received_len = recv(client_socket, server_reply, sizeof(server_reply), 0);
        if (received_len < 0){
            perror("receive error");
        }

        //Parse the hostname and subdirectories from client request
        parse_fqdn_subdir_clientport();
        hostname = fqdn;

        if (is_banned(fqdn)){
            message = "403 Forbidden\n";
            write(client_socket, message, strlen(message));
            close(client_socket);
            break;
        }

        // concatenate url
        char url[512];
        strcpy(url,fqdn);
        strcat(url,subdir);
        hash_file_name(url);
    
        //Check if the cache file already exists
        if( access(hashed_file_name, F_OK ) != -1 ) 
        {
            puts("file exists"); 
            // pull data from file
            int file_c;
            FILE *cache_file;
            cache_file = fopen(hashed_file_name, "r");
            char cached_data[4096];
            int ctr = 0;
            if (cache_file) {
                puts("HTTP/1.1 200 OK");
                while ((file_c = getc(cache_file)) != EOF)
                {
                    cached_data[ctr] = file_c;
                    ctr++;
                    if(ctr == 4095)
                    {
                        send(client_socket, cached_data, strlen(cached_data), 0);
                        bzero((char*) cached_data, 4096);
                        ctr = 0;
                    }					
                }
                fclose(cache_file);
            }
        send(client_socket, cached_data, strlen(cached_data), 0);
        bzero((char*) cached_data, 4096);
	    } 
	    else 
	    {
        // file doesn't exist
        // request data from server
        if(strcmp(request_type, "GET") != 0){
            printf("%s 405 Method Not Allowed\n",protocol);
            close(client_socket);
            return 0;
        }

        if ( (he = gethostbyname( hostname )) == NULL){
            message = "HTTP/1.1 500 Internal Server Error\n";
            send(client_socket, message, strlen(message), MSG_WAITALL);
            close(client_socket);
            return 0;
        }

        addr_list = (struct in_addr **) he->h_addr_list;
        int i;
        for (i = 0; addr_list[i] != NULL; i++){
            strcpy(ip, inet_ntoa(*addr_list[i]));
        }

        client = bind_socket(AF_INET, inet_addr(ip), htons(client_port));
        host_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(host_socket, (struct sockaddr *) &client, sizeof(client)) < 0){
            puts("Connect error");
            return 0;
        }
		
        if (is_banned(fqdn)){
            message = "403 Forbidden";
            return 0;
        }

        setsockopt(host_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
        
        sprintf(buffer, "GET /%s HTTP/1.1\nHost: %s\r\n\r\n",subdir,fqdn);
        if ( send(host_socket, buffer, strlen(buffer), 0) < 0){
            puts("Send failed");
        }
        
        while (recv(host_socket, server_reply, 4096, 0) > 0){ 
            if (responseStart){
                //Receive HTTP header
                strcpy(http_header,server_reply);
                strptr = strtok(http_header, "\r\n\r\n");
                strcpy(http_header, strptr);
                puts(http_header);
                responseStart = 0;
                if (strcmp(http_header, "HTTP/1.1 200 OK") != 0) {
                        badResponse = 1;
                }
            }
            if (badResponse) {
                strcat(http_header, "\n");
                send(client_socket, http_header, strlen(http_header), MSG_WAITALL);
                break;
            }
            fp = fopen(hashed_file_name, "ab+");
            //write in cache file
            fprintf(fp, "%s\n", server_reply);
            //REDIRECT the requested page content to client
            send(client_socket, server_reply, strlen(server_reply), 0);
            bzero((char*) server_reply, 4096);
        }
        close(host_socket);
		if (!badResponse){
            fclose(fp);
        }
	}
    message = "\n";
    send(client_socket, message, strlen(message), 0);
    close(client_socket);
    }
}
