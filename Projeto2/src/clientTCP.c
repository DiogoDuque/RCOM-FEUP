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


/*
 * Handles everything related to initializing communications through the sockets
 * @param SERVER_ADDR - IP
 * @param SERVER_PORT - Port
 * @return - socket file descriptor
 */
int initSocket(char* SERVER_ADDR, int SERVER_PORT){
	int sockfd;
	char response[256];
	struct sockaddr_in server_addr;
	
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
   	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
       	perror("connect()");
		exit(0);
	}
	memset(response, 0, 256);
	read(sockfd, response, 50);
	printf("Server response: %s\n", response);

	return sockfd;
}


/*
 * 
 * @param hostname - hostname to convert to IP
 * @return - IP associated to hostname
 */
char* getIP(char* hostname) {
	struct hostent *h;

    if((h=gethostbyname(hostname)) == NULL) {  
        herror("gethostbyname");
        exit(1);
    }

    return inet_ntoa(*((struct in_addr *)h->h_addr));
}


/*
 * Sends a command to the server and awaits for a response.
 * @param sockfd - socket file descriptor
 * @param cmd - cmd to be sent to server
 * @param response - response from the server
 */
void interact(int sockfd, char* cmd, char* response){
	char separator[] = "-----------------------\n\n";;

	int bytes = write(sockfd, cmd, strlen(cmd));
	printf("User cmd: %s\n",cmd);
	if(bytes != strlen(cmd))
		printf("Warning: cmd may have not been fully sent...\n");

	memset(response, 0, 256);
	read(sockfd, response, 50);
	printf("Server response: %s\n", response);
	printf("%s",separator);
}


int main(int argc, char** argv){ // ftp://ftp.up.pt/pub/robots.txt

	if(argc != 2){
		perror("Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
		exit(0);
	}


	int	sockfd, sockfd2, bytes, SERVER_PORT = 21;
	struct sockaddr_in server_addr;
	char hostname[128], path[128], user[128], pass[128], cmd[128], response[128], *SERVER_ADDR;


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
		strcpy(pass, "1213456789");
	}else {
		perror("Invalid URL! Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
		exit(0);
	}
	SERVER_ADDR = getIP(hostname);
	printf("IP Address : %s\n\n",SERVER_ADDR);
	

	/*init socket*/
	sockfd=initSocket(SERVER_ADDR, SERVER_PORT);


   	/*send user*/
	strcpy(cmd, "USER ");
	strcat(cmd, user);
	strcat(cmd, "\n");
	interact(sockfd,cmd,response);
	

	/*send pass*/
	strcpy(cmd, "PASS ");
	strcat(cmd, pass);
	strcat(cmd, "\n");
	interact(sockfd,cmd,response);


	/*pasv*/
	int pasv[6]; //ip[4]+port[2]
	strcpy(cmd, "PASV");
	strcat(cmd, "\n");
	interact(sockfd,cmd,response);
	sscanf(response, "%*[^(](%d,%d,%d,%d,%d,%d)\n", &pasv[0], &pasv[1], &pasv[2], &pasv[3], &pasv[4], &pasv[5]);
	
	/*get info about port on which server will be listening*/
	sprintf(SERVER_ADDR, "%d.%d.%d.%d", pasv[0], pasv[1], pasv[2], pasv[3]);
	SERVER_PORT=pasv[4]*256 + pasv[5];
	printf("NEW SOCKET:\nIP: %s\nPORT: %d\n",SERVER_ADDR,SERVER_PORT);

	/*open socket for the server listener*/
	sockfd2=initSocket(SERVER_ADDR, SERVER_PORT);
	//...



	/*ask for the file*/
	strcpy(cmd, "RETR ");
	strcat(cmd, path);
	strcat(cmd, "\n");
	interact(sockfd,cmd,response);


	close(sockfd);
	exit(0);
}


