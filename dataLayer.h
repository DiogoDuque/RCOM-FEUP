#include <unistd.h>

#define TRANSMITTER 0
#define RECEIVER 1

typedef enum {START, FIRST_F, READING, LAST_F} state;

struct termios oldtio,newtio;

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


int init(char* port) {
	int fd = open(port, O_RDWR | O_NOCTTY );
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
	return fd;
}

char receiveMessage(int fd) {
	char buf[16];
	buf[0]='\0';
    int res = read(fd,buf,1);

   	//buf[res]='\0';          /* so we can printf... */

	int res2=write(fd,buf,1);

	return buf[0];
}

//tries to send a message. if was properly sent, returns TRUE; otherwise returns FALSE
int sendMessage(int fd, char* msg) {

	int size = strlen(msg);
    int res = write(fd,msg,size);
    //printf("writing: «%s»; written %d of %d written\n\n\n", msg,res,size);

	if(res==size) return TRUE;
	else return FALSE;
}

void sendSET(int fd){ //cmd
	char msg[5];
	msg[0]=0x7E; //F
	msg[1]=0x03; //A
	msg[2]=0x03; //C
	msg[3]=0x04; //BCC1
	msg[4]=0x7E; //F
	sendMessage(fd,msg);
}

void sendUA(int fd, int mode){ //ans
	char msg[5];
	msg[0]=0x7E; //F
	msg[1]=mode==TRANSMITTER?0x01:0x03; //A
	msg[2]=0x07; //C
	msg[3]=0x00; //BCC1
	msg[4]=0x7E; //F
	sendMessage(fd,msg);
}

void sendDISC(int fd, int mode){ //cmd
	char msg[5];
	msg[0]=0x7E; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=0x03; //C
	msg[3]=0x04; //BCC1
	msg[4]=0x7E; //F
	sendMessage(fd,msg);
}

int stateMachineSET(int fd) {
	char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);

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

			if(msg[2]==0x03) { //ANALYZE C
				return TRUE;
			}
			else {
				printf("C is incorrect. expected 0x03, was 0x%02X\n",msg[2]);
				return FALSE;
			}

		default:
			printf("ERROR: NO STATE FOUND\n");
			return FALSE;
		}

		printHex(msg);
	}
}

int stateMachineUA(int fd) {
	char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);

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

			if(msg[2]==0x07) //ANALYZE C
				return TRUE;
			else {
				printf("C is incorrect. expected 0x03, was 0x%02X\n",msg[2]);
				return FALSE;
			}

		default:
			printf("ERROR: NO STATE FOUND\n");
			return FALSE;
		}

		printHex(msg);
	}
}

int llopen(char* port, int mode) {
	int fd = init(port);
	if(mode == RECEIVER) {
		if(stateMachineSET(fd)==TRUE) {
			sendUA(fd,mode);
			printf("SET received successfully!\n");
			return fd;
		} else {
			printf("SET was not received successfully\n");
			return -1;
		}
	} else if(mode == TRANSMITTER) {
		sendSET(fd);
		if(stateMachineUA(fd)==TRUE) {
			printf("UA received successfully!\n");
			return fd;
		} else {
			printf("UA was not received successfully\n");
			return -1;
		}
	} else return -2;
}

int llwrite(int fd, char* buffer, int length){

}

int llread (int fd, char* buffer) {

}

int llclose(int fd) {
	int ret=0;
	if(tcsetattr(fd,TCSANOW,&oldtio) != 0)
		ret += 2;
	if(close(fd) != 0)
		ret += 1;
	return ret;
}

