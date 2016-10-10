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

   	//printf("received:%s:%d\n", buf, res);

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
typedef enum {START, FIRST_F, READING, LAST_F} state;

int stateMachine() {
	char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		char info;
		if(st!=LAST_F) {
		 	info=receiveMessage();

			if(info == '\0')
				return FALSE;
			printf("received 0x%02X\n",info);
		}

		switch(st) {

		case START:
			printf("START\n");
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
			printf("FIRST_F\n");
			if(info==0x7E)
				st=FIRST_F;
			else {
				st=READING;
				msg[0]=0x7E;
				msg[1]=info;
				counter=2;
			}
			break;

		case READING:
			printf("READING\n");
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			printf("LAST_F\n");
			if(counter != 5) { //ANALYZE STRLEN
				printf("length is incorrect. expected 5, was %lu\n",strlen(msg));
				return FALSE;
			}

			if(msg[3] != (msg[1]^msg[2])) { //ANALYZE BCC1
				printf("BCC1 != (A XOR C)\n");
				return FALSE;
			}

			if(msg[2]==0x03) //ANALYZE C
				sendUA();
			else {
				printf("C is incorrect. expected 0x03, was 0x%02X\n",msg[2]);
				return FALSE;
			}
		
			sendUA();
			return TRUE;


		default:
			printf("ERROR: NO STATE FOUND\n");
			return FALSE;
		}

		printHex(msg);
	}
}


void llopen() {
	while(stateMachine() != TRUE){
		printf("RESTARTING STATE MACHINE\n");
	}
	printf("EXITED STATE MACHINE\n");
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

	llopen();

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
