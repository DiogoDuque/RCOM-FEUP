#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>

#define SERVER_PORT 21


char* getIP(char* hostname) {

	struct hostent *h;

    if((h=gethostbyname(hostname)) == NULL) {  
        herror("gethostbyname");
        exit(1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n\n",inet_ntoa(*((struct in_addr *)h->h_addr)));
    return inet_ntoa(*((struct in_addr *)h->h_addr));
}

void getResponse(int sockfd){
	char response[256];
	int bytes;

	memset(response, 0, 256);
	bytes = read(sockfd, response, 50);
	printf("bytes: %d\n",bytes);
	printf("Server response: %s\n", response);
}


int main(int argc, char** argv){ // ftp://ftp.up.pt/pub/robots.txt

	if(argc != 2){
		perror("Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
		exit(0);
	}


	int	sockfd;
	struct sockaddr_in server_addr;
	char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;
	char hostname[128], path[128], user[128], pass[128];


	/*get info from url (with reg expr)*/
	if(sscanf(argv[1], "ftp://%[^:]:%[^@]@%[^/]/%s\n", user, pass, hostname, path) == 4){
		printf("Host: %s\n", hostname);
		printf("Path: %s\n", path);
		printf("User: %s\n", user);
		printf("Pass: %s\n", pass);
		}else if(sscanf(argv[1], "ftp://%[^/]/%s\n", hostname, path) == 2){
		printf("Host: %s\n", hostname);
		printf("Path: %s\n", path);
		strcpy(user, "Anonymous");
		strcpy(pass, "password");
	}else {
		perror("Invalid URL! Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
		exit(0);
	}
	char* SERVER_ADDR = getIP(hostname);
	

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */
    

	/*open a TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
   	}


	/*connect to the server*/
   	if(connect(sockfd, 
           (struct sockaddr *)&server_addr, 
		   sizeof(server_addr)) < 0){
       	perror("connect()");
		exit(0);
	}


   	/*send a string to the server*/
	bytes = write(sockfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);

	getResponse(sockfd);

	close(sockfd);
	exit(0);
}


