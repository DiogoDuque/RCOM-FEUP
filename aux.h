#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

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
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n\n");
}

void printHex(char* hexMsg) {
	char hexArray[255];
	strcpy(hexArray,hexMsg);
	printf("HEX ARRAY(%lu):",strlen(hexArray));

	int i;
	for(i=0; i<strlen(hexMsg); i++) {
		printf(" 0x%02X",hexArray[i]);
	}
	printf("\n\n");
}

char receiveMessage() {
	char buf[16];
	buf[0]='\0';
    res = read(fd,buf,1);   

   	buf[res]='\0';          /* so we can printf... */

	int res2=write(fd,buf,1);

	return buf[0];
}

//tries to send a message. if was properly sent, returns TRUE; otherwise returns FALSE
int sendMessage(char* msg) {

	int size = strlen(msg);
    int res = write(fd,msg,size);
    printf("writing: «%s»; written %d of %d written\n\n\n", msg,res,size);

	if(res==size) return TRUE;
	else return FALSE;


	/*char buf[255];
	while(TRUE) {
		int res2 = read(fd, buf,1);
		buf[res2]='\0';
		printf("received:%s:%d\n\n", buf, res2);

		if(strcmp(buf,"") == 0)
			break;

	}*/

}
