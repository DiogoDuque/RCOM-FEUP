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
	}/*
	memset(response, 0, 256);
	read(sockfd, response, 50);
	printf("Server response: %s\n", response);
*/
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
	read(sockfd, response, 100);
	printf("Server response: %s\n", response);
	printf("%s",separator);
}

/*
 * Gets the filename from path
 */
char* getFilename(char* path){
	int it=0;
	char* retr;
	int started=0;
	while(path[it]!='\0'){
		if(path[it]=='/')retr=&path[it+1];
		++it;
	}
	return retr;
}

int lastIndexOf(const char * s, char target)
{
   int ret = -1;
   int curIdx = 0;
   while(s[curIdx] != '\0')
   {
      if (s[curIdx] == target) ret = curIdx;
      curIdx++;
   }
   return ret;
}

int getFileSize(const char* response) {
	int index = lastIndexOf(response, '(') + 1;
	int length = lastIndexOf(response, ')') - index - 6;
	char substr[length];
	memcpy(substr, &response[index], length);
	substr[length] = '\0';
	return atoi(substr);
	
}

void printPercentage(int current, int total) {
	float p = (float)current / (float)total;
	int perc = p * 100;
	if (p >= 1) printf("\r[--------------------] (%d%)", perc);
	else if (p > 0.95) printf("\r[------------------- ] (%d%)", perc);
	else if (p > 0.90) printf("\r[------------------  ] (%d%)", perc);
	else if (p > 0.85) printf("\r[-----------------   ] (%d%)", perc);
	else if (p > 0.80) printf("\r[----------------    ] (%d%)", perc);
	else if (p > 0.75) printf("\r[---------------     ] (%d%)", perc);
	else if (p > 0.70) printf("\r[--------------      ] (%d%)", perc);
	else if (p > 0.65) printf("\r[-------------       ] (%d%)", perc);
	else if (p > 0.60) printf("\r[------------        ] (%d%)", perc);
	else if (p > 0.55) printf("\r[-----------         ] (%d%)", perc);
	else if (p > 0.50) printf("\r[----------          ] (%d%)", perc);
	else if (p > 0.45) printf("\r[---------           ] (%d%)", perc);
	else if (p > 0.40) printf("\r[--------            ] (%d%)", perc);
	else if (p > 0.35) printf("\r[-------             ] (%d%)", perc);
	else if (p > 0.30) printf("\r[------              ] (%d%)", perc);
	else if (p > 0.25) printf("\r[-----               ] (%d%)", perc);
	else if (p > 0.20) printf("\r[----                ] (%d%)", perc);
	else if (p > 0.15) printf("\r[---                 ] (%d%)", perc);
	else if (p > 0.10) printf("\r[--                  ] (%d%)", perc);
	else if (p > 0.05) printf("\r[-                   ] (%d%)", perc);
	else printf("\r[                    ] (%d%)", perc);
}

// ftp://ftp.up.pt/pub/robots.txt
// ftp://ftp.up.pt/pub/fedora-epel/fullfilelist
int main(int argc, char** argv){ 
	char hostname[128], path[128], user[128], pass[128];

	/*if(argc != 2){
		perror("Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
		exit(0);
	}*/

	if (argc == 1){
		char* pos;
		printf("Enter host: ");
		fgets(hostname, sizeof(hostname), stdin);
		if ((pos=strchr(hostname, '\n')) != NULL) *pos = '\0';

		printf("Enter host path: ");
		fgets(path, sizeof(path), stdin);
		if ((pos=strchr(path, '\n')) != NULL) *pos = '\0';

		printf("Enter username (empty for Guest): ");
		fgets(user, sizeof(user), stdin);
		if ((pos=strchr(user, '\n')) != NULL) *pos = '\0';
		
		if (strlen(user) == 0) {
			strcpy(user, "anonymous");
			strcpy(pass, " ");
		} else {
			printf("Enter password: ");
			fgets(pass, sizeof(pass), stdin);
			if ((pos=strchr(pass, '\n')) != NULL) *pos = '\0';
		}
		printf("Host: %s\nHost path: %s\nUser: %s\n", hostname, path, user);
	} else if (argc == 2) {
		/*get info from url (with reg expr)*/
		if(sscanf(argv[1], "ftp://%[^:]:%[^@]@%[^/]/%s\n", user, pass, hostname, path) == 4){
			printf("Host: %s\n", hostname);
			printf("Path: %s\n", path);
			printf("User: %s\n", user);
			printf("Pass: %s\n", pass);
		}else if(sscanf(argv[1], "ftp://%[^/]/%s\n", hostname, path) == 2){
			printf("Host: %s\n", hostname);
			printf("Path: %s\n", path);
			strcpy(user, "anonymous");
			strcpy(pass, " ");
		}else {
			perror("Invalid URL! Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
			exit(0);
		}
	} else {
		perror("Invalid arguments! Usage: ftp ftp://[<user>:<password>@]<host>/<url-path>");
		exit(0);
	}
	char* filename = getFilename(path);
	int	sockfd, sockfd2, bytes, SERVER_PORT = 21;
	struct sockaddr_in server_addr;
	char cmd[128], response[128], *SERVER_ADDR;

	
	SERVER_ADDR = getIP(hostname);
	printf("IP Address : %s\n\n",SERVER_ADDR);
	

	/*init socket*/
	sockfd=initSocket(SERVER_ADDR, SERVER_PORT);

    memset(response, 0, 256);
	read(sockfd, response, 50);
	printf("Server response: %s\n", response);

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
    printf("NEW SOCKET:\nIP: %s\nPORT: %d\n\n",SERVER_ADDR,SERVER_PORT);

    //specify type (binary/image)
    strcpy(cmd, "TYPE I\n");
    interact(sockfd, cmd, response);

    /*open socket for the server listener*/
	sockfd2=initSocket(SERVER_ADDR, SERVER_PORT);
	//...

    /*ask for the file*/
    strcpy(cmd, "RETR ");
    strcat(cmd, path);
	strcat(cmd, "\n");
	interact(sockfd,cmd,response);


	FILE * file = fopen(filename, "w");
	int filesize = getFileSize(response);
    memset(response, 0, 256);

    int len;
	int pak = 0;
	printf("Downloading file: %s\n", filename);
    while ((len = read(sockfd2, response, 255)) > 0) {
		printPercentage(pak++*255, filesize);
		fwrite(response,sizeof(char),len,file);
	}
	printf("\n\n");
	
    strcpy(cmd, "QUIT\n");
	interact(sockfd,cmd,response);

    close(sockfd2);
	close(sockfd);
	exit(0);
}


