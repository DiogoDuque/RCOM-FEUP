/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

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

    printf("New termios structure set\n");
}

//tries to send a message. if was properly sent, returns TRUE; otherwise returns FALSE
int sendMessage(char* msg) {

	int size = strlen(msg)+1;
    int res = write(fd,msg,size);
    printf("writing: «%s»; written %d of %d written\n\n\n", msg,res,size);

	sleep(1); //assim nao le o que acabou de enviar

	char buf[255];
	while(TRUE) {
		int res2 = read(fd, buf,1);
		buf[res2]='\0';
		printf("received:%s:%d\n\n", buf, res2);

		if(strcmp(buf,"") == 0)
			break;

	}

}

int main(int argc, char** argv) {
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }
    
	init(argv[1]);

	char msg[255];
	msg[0]=0x7E;
	msg[1]=0x7E;	
	msg[2]=0x03;
	msg[3]=0x04;
	msg[4]=0x75;
	msg[5]=0x7E;
	//while(TRUE) {
		sendMessage(msg);
	//}
	
 

  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */



   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
