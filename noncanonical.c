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
	char buf[16];
	buf[0]='\0';
    res = read(fd,buf,1);   

   	buf[res]='\0';          /* so we can printf... */
   	//printf("received:%s:%d\n", buf, res);

	int res2=write(fd,buf,1);
	//printf("bytes written:%d\n\n",res2);

	return buf[0];
}

void printHex(char* hexMsg) {
	char hexArray[255];
	strcpy(hexArray,hexMsg);
	printf("HEX ARRAY (%lu):",strlen(hexArray));

	int i;
	for(i=0; i<strlen(hexMsg); i++) {
		printf(" 0x%02X",hexArray[i]);
	}
	printf("\n\n");
}

/*
	STATE MACHINE
*/
int stateFirst7E();
int stateReading();
int stateLast7E();

int stateStart() {
	char info = receiveMessage();
	if(info == '\0')
		return FALSE;

	printf("start: 0x%02X\n",info);

	if(info=!0x7E)
		return stateStart();
	else return stateFirst7E();
}

int stateFirst7E() {
	char info = receiveMessage();
	if(info == '\0')
		return FALSE;

	printf("first7E: 0x%02X\n",info);

	if(info==0x7E)
		return stateFirst7E();
	else {
		char msg[255];
		msg[0]=0x7E;
		return stateReading(msg);
	}
}

int stateReading(char* msg) {
	char info = receiveMessage();
	if(info == '\0')
		return FALSE;

	printf("reading: 0x%02X\n",info);
	printHex(msg);

	//char msg2[255];
	//strcpy(msg2,msg);
	char infoArray[16];
	infoArray[0]=info;
	strcat(msg,infoArray);
	if(info=!0x7E)
		return stateReading(msg);
	else return stateLast7E(msg);
}

int stateLast7E(char* msg) {
	char lastHex[4];
	lastHex[0]=0x7E;
	strcat(msg,(char*)lastHex);
	printHex(msg);
	return TRUE; //NOT FINISHED
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

	stateStart();
	printf("EXITED STATE MACHINE\n");

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
