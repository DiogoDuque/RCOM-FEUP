/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


int fd,c, res;
struct termios oldtio,newtio;

void init(char* port) {

    fd = open(port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(port); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

}

char receiveMessage() {
	char buf[1];
	buf[0]='\0';
    res = read(fd,buf,1);

   	printf("received:%s:%d\n", buf, res);

	//int res2=write(fd,buf,1);
	//printf("bytes written:%d\n\n",res2);

	if(buf[0]=='\0'){
		res = read(fd,buf,1); printf("READ \\0\n");}

	return buf[0];
}

void sendUA(){
	printf("----------------\nwill send UA when it's done!\n----------------\n");
}

void printHex(char* hexMsg) {
	char hexArray[255];
	strcpy(hexArray,hexMsg);
	printf("HEX ARRAY(%lu):",strlen(hexArray));

	int i, size=strlen(hexArray);
	for(i=0; i<size; i++) {
		printf(" 0x%02X",hexArray[i]);
	}
	printf("\n\n");
}

/*
	STATE MACHINE
*/
int stateFirstF();
int stateReading(char* msg);
int stateLastF(char* msg);

int stateStart() {
	char info = receiveMessage();
	if(info == '\0')
		return FALSE;

	printf("start: 0x%02X\n",info);

	if(info!=0x7E)
		return stateStart();
	else return stateFirstF();
}

int stateFirstF() {
	char info = receiveMessage();
	if(info == '\0')
		return FALSE;

	printf("first7E: 0x%02X\n",info);

	if(info==0x7E)
		return stateFirstF();
	else {
		char msg[255];
		msg[0]=0x7E;
		msg[1]=info;
		return stateReading(msg);
	}
}

int stateReading(char* msg) {
	char info = receiveMessage();
	if(info == '\0')
		return FALSE;

//CAT
	char infoArray[1];
	infoArray[0]=info;
	strcat(msg,infoArray);

//PRINT
	printf("reading: 0x%02X; ",info);
	printHex(msg);

//NEXT STATE
	if(info!=0x7E)
		return stateReading(msg);
	else return stateLastF(msg);
}

int stateLastF(char* msg) {
//PRINT	
	printf("full message: ");
	printHex(msg);

//ANALYZE LEN
	if(strlen(msg) != 5) {
		printf("length is incorrect. expected 5, was %lu\n",strlen(msg));
		return FALSE;
	}

//ANALYZE BCC1
	if(msg[3] != (msg[1]^msg[2])) {
		printf("BCC1 != (A XOR C)\n");
		return FALSE;
	}

//ANALYZE C
	if(msg[2]==0x03)
		sendUA();
	else {
		printf("C is incorrect. expected 0x03, was 0x%02X\n",msg[2]);
		return FALSE;
	}
	
	return TRUE;
}

int main(int argc, char** argv)
{
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	init(argv[1]);

	while(stateStart() != TRUE) {
		printf("RESTARTING STATE MACHINE\n");
	}
	printf("EXITED STATE MACHINE\n");

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
