/*Non-Canonical Input Processing*/
#include "aux.h"
#include "dataLayer.h"

typedef enum {START, FIRST_F, READING, LAST_F} state;

void sendUA(){
	printf("----------------\nwill send UA when it's done!\n----------------\n");
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


void llopen(int fd) {
	while(stateMachineSET(fd) != TRUE){
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

	int fd = init(argv[1]);

	llopen(fd);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
