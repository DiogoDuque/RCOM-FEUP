#include <unistd.h>
#include <fcntl.h>

#define TRANSMITTER 0
#define RECEIVER 1

typedef enum {START, FIRST_F, READING, LAST_F} state;

struct termios oldtio,newtio;

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE   0
#define TRUE    1

#define FLAG    0x7E
#define ESCAPE  0x7D
#define ESC7E   0x5E
#define ESC7D   0x5D

//various values for 'Control' field
#define INF0    0x00
#define INF1    0x40
#define SET     0x03
#define DISC    0x0b
#define UA      0x07
#define RR0     0x05
#define RR1     0x85
#define REJ0    0x01
#define REJ1    0x81


int mode;
int Nr = 0;

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
    int res;
	while((res = read(fd,buf,1)) <= 0);
   	//buf[res]='\0';          /* so we can printf... */

	return buf[0];
}

//tries to send a message. if was properly sent, returns TRUE; otherwise returns FALSE
int sendMessage(int fd, char* msg, int size) {

    int res = write(fd,msg,size);
    //printf("writing: «%s»; written %d of %d written\n\n\n", msg,res,size);

	if(res==size) return TRUE;
	else return FALSE;
}

void sendSET(int fd){ //cmd
	char msg[5];
	msg[0]=FLAG; //F
	msg[1]=0x03; //A
	msg[2]=0x03; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	printHex(msg,5);
	if(sendMessage(fd,msg,5)==TRUE)
		printf("SET sent successfully!\n");
	else printf("Warning: SET was not sent successfully!\n");
}

void sendUA(int fd){ //ans
	char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x01:0x03; //A
	msg[2]=0x07; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	printHex(msg,5);
	if(sendMessage(fd,msg,5)==TRUE)
		printf("UA sent successfully!\n");
	else printf("Warning: UA was not sent successfully!\n");
}

void sendDISC(int fd){ //cmd
	char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=0x0B; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	if(sendMessage(fd,msg,5)==TRUE)
		printf("DISC sent successfully!\n");
	else printf("Warning: DISC was not sent successfully!\n");
}

void sendRR(int fd){
    char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=Nr==0?RR0:RR1; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	if(sendMessage(fd,msg,5)==TRUE)
		printf("RR sent successfully!\n");
	else printf("Warning: RR was not sent successfully!\n");
}

void sendREJ(int fd){
    char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=Nr==0?REJ1:REJ0; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	if(sendMessage(fd,msg,5)==TRUE)
		printf("REJ sent successfully!\n");
	else printf("Warning: REJ was not sent successfully!\n");
}

int stateMachineSET(int fd) {
	char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);
			//printf("received 0x%02X\n",info);
		}

		switch(st) {

		case START:
			//printf("START\n");
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
			//printf("FIRST_F\n");
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
			//printf("READING\n");
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			//printf("LAST_F\n");
			printf("RECEIVING SET: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING SET: length is incorrect. expected 5, was %lu\n",strlen(msg));
				return FALSE;
			}

			if(msg[3] != (msg[1]^msg[2])) { //ANALYZE BCC1
				printf("RECEIVING SET: BCC1 != (A XOR C)\n");
				return FALSE;
			}

			if(msg[2]==0x03) { //ANALYZE C
				return TRUE;
			}
			else {
				printf("RECEIVING SET: C is incorrect. expected 0x03, was 0x%02X\n",msg[2]);
				return FALSE;
			}

		default:
			printf("RECEIVING SET: NO STATE FOUND\n");
			return FALSE;
		}
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
			if(alarmFlag) return FALSE;
			}
			//printf("received 0x%02X\n",info);

		switch(st) {

		case START:
			//printf("START\n");
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
			//printf("FIRST_F\n");
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
			//printf("READING\n");
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			//printf("LAST_F\n");
			printf("RECEIVING UA: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING UA: length is incorrect. expected 5, was %lu\n",strlen(msg));
				return FALSE;
			}

			if(msg[3] != (msg[1]^msg[2])) { //ANALYZE BCC1
				printf("RECEIVING UA: BCC1 != (A XOR C)\n");
				return FALSE;
			}

			if(!(msg[1]==0x03 && mode==RECEIVER) &&
				!(msg[1])==0x01 && mode==TRANSMITTER){
				printf("RECEIVING UA: A is incorrect\n");
				return FALSE;
			}

			if(msg[2]==0x07) //ANALYZE C
				return TRUE;
			else {
				printf("RECEIVING UA: C is incorrect. expected 0x07, was 0x%02X\n",msg[2]);
				return FALSE;
			}

		default:
			printf("RECEIVING UA: NO STATE FOUND\n");
			return FALSE;
		}
	}
}

int stateMachineDISC(int fd) {
	char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);
			if(alarmFlag) return FALSE;
			}
			//printf("received 0x%02X\n",info);

		switch(st) {

		case START:
			//printf("START\n");
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
			//printf("FIRST_F\n");
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
			//printf("READING\n");
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			//printf("LAST_F\n");
			printf("RECEIVING DISC: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING DISC: length is incorrect. expected 5, was %lu\n",strlen(msg));
				return FALSE;
			}

			if(msg[3] != (msg[1]^msg[2])) { //ANALYZE BCC1
				printf("RECEIVING DISC: BCC1 != (A XOR C)\n");
				return FALSE;
			}

			if(!(msg[1]==0x03 && mode==TRANSMITTER) &&
				!(msg[1])==0x01 && mode==RECEIVER){
				printf("RECEIVING DISC: A is incorrect\n");
				return FALSE;
			}

			if(msg[2]==0x0B) //ANALYZE C
				return TRUE;
			else {
				printf("RECEIVING DISC: C is incorrect. expected 0x0B, was 0x%02X\n",msg[2]);
				return FALSE;
			}

		default:
			printf("RECEIVING DISC: NO STATE FOUND\n");
			return FALSE;
		}
	}
}

int llopen(char* port, int flag) {
	mode = flag;
	int fd = init(port);
	if(mode == RECEIVER) {
		if(stateMachineSET(fd)==TRUE) {
			printf("SET received successfully!\n");
			sendUA(fd);
			return fd;
		} else {
			printf("SET was not received successfully\n");
			return -1;
		}
	} else if(mode == TRANSMITTER) {
		sendSET(fd);
		int flags=fcntl(fd, F_GETFL, 0); //read is in nonblock so
		fcntl(fd, F_SETFL, flags | O_NONBLOCK); //alarm works
		if(stateMachineUA(fd)==TRUE) {
			printf("UA received successfully!\n");
			fcntl(fd, F_SETFL, flags);
			return fd;
		} else {
			printf("UA was not received successfully\n");
			fcntl(fd, F_SETFL, flags);
			return -1;
		}
	} else return -2;
}

int llwrite(int fd, char* buffer, int length) {
	int size = length + 6;
	char package [2*length + 6];
	char A = 0x03;
	char C = 0x00;
	char BCC2 = 0x00;

	int i;
	int offset = 4;
	for (i = 0; i < length; i++) {
		C = C^0x01;
		char byte = buffer[i];

		// Stuffing
		if (byte == 0x7E) {
			package[i + offset++] = 0x7D;
			package[i + offset] = 0x5E;
			size++;
		} else if (byte == 0x7D) {
			package[i + offset++] = 0x7D;
			package[i + offset] = 0x5D;
			size++;
		} else {
			package[i + offset] = buffer[i];
		}
	}

	package[0] = FLAG;
	package[1] = A;
	package[2] = C;
	package[3] = package[1]^package[2];
	package[i++] = FLAG;

	return sendMessage(fd, package,i+1);
}

/**
*   calculates expected bcc value from given buffer
*   @param  buf     buffer of bytes on which bcc will be calculated
*   @param  bufLength   length of buf array
*   @return bcc's expected value
*/
char calcBCC(char * buf, int bufLength){
    int i = 0;
    char res = 0x00;
    for (i = 0; i < bufLength; i++){
        res ^= buf[i];
    }
    return res;
}

struct Trama {
    char address;
    char control;
    char bcc1;
    char data[255];
    int dataLength;
    char bcc2;
};

/**
*   reads fd's content and loads trama's fields accordingly
*   @param fd       file descriptor
*   @param trama    empty Trama to fill with information from fd
*   @return trama's size, in bytes
*/
int readTrama(int fd, struct Trama * trama){
    int nrBytes = 0;
    int r;
    char header[4];
    char c;
    int i = 0;

    r = read(fd, &header, 4);    //4 = header length (in bytes)

    if (r < 4){
        perror("error on read (readTrama)\n");
        return -1;
    }

    //filling header
    trama->address = header[1];
    trama->control = header[2];
    trama->bcc1 =    header[3];

    //data field loop

    if (trama->control == INF0 || trama->control == INF1){
        while (read(fd, &c, 1) == 1){

            if (c == FLAG){
            	if (i != 0){
            		trama->bcc2 = trama->data[i-1];
		            trama->data[i-1] = 0;
		            printf("final i value: %d\n", i);
		            return i-1;     //real trama length is i+escape_offset-1+6 = i+5
            	}
            }

            //destuffing
            if (c == ESCAPE){
                if (read(fd, &c, 1) < 1){
                    perror("error while reading data\n");
                    return 1;
                }

                if (c == ESC7E){
                    trama->data[i] = FLAG;
                }
                if (c == ESC7D){
                    trama->data[i] = ESCAPE;
                }
            }
            else {
                trama->data[i] = c;
            }
            i++;
        }
    }

    char str[18];
    sprintf(str, "error on read (i=%d)\n", i);

    perror(str);
    return -1;
}

int llread (int fd, char* buffer) {
    struct Trama trama;

    while(TRUE) {
        int tramaLength = readTrama(fd, &trama);
        if (tramaLength < 5){   //least amount of memory a trama will need
            perror("error on readTrama\n");
            return -1;  //Tramas I, S ou U com cabecalho errado sao ignoradas, sem qualquer accao (1)
        }

        if (trama.address^trama.control == trama.bcc1){    //i.e. if header is correct
            //llread does will never receive tramas of type RR or REJ
            switch(trama.control){
                case SET:
                    sendUA(fd);
                    break;
                case UA:
                    //file transfer over
                    //...
                    break;
                case INF0:  //Ns = 0
                    if (Nr == 1){   //data is not duplicate
                        if (calcBCC(trama.data, trama.dataLength) == trama.bcc2){   //data bcc is correct
                            //accept trama
                            //...
                            printHex(trama.data, trama.dataLength);

                            sendRR(fd);
                            Nr = 0;
                        }
                        else {
                            sendREJ(fd);
                        }
                    }
                    else {
                        sendRR(fd);
                    }
                    break;
                case INF1:  //Ns = 1
                    if (Nr == 0){   //data is not duplicate
                        if (calcBCC(trama.data, trama.dataLength) == trama.bcc2){   //data bcc is correct
                            //accept trama

                            sendRR(fd);
                            Nr = 1;
                        }
                        else {
                            sendREJ(fd);
                        }
                    }
                    else {
                        sendRR(fd);
                    }
                    break;
                case DISC:
                    sendDISC(fd);
                    break;
                default:
                    perror("invalid CONTROL value\n");
                    return -1;
            }
            return tramaLength;
        }
        else {  //(1)
            perror("wrong header bcc\n");
            return -1;
        }
    }
}

int llclose(int fd) {
	printf("Starting to close...\n\n");

	if(mode==TRANSMITTER) { /* ---------MODE RECEIVER--------- */

		//send DISC
		sendDISC(fd);

		//receive DISC
		if(stateMachineDISC(fd)==TRUE)printf("DISC received successfully!\n");
		else {printf("DISC was not received successfully!\n"); return -2;}

		//send UA
		sendUA(fd);

	} else { /* -------------MODE RECEIVER------------- */

		//receive DISC
		if(stateMachineDISC(fd)==TRUE) printf("DISC received successfully!\n");
		else {printf("DISC was not received successfully!\n"); return -1;}

		//send DISC
		sendDISC(fd);

		//send UA
		if(stateMachineUA(fd)==TRUE) printf("UA received successfully!\n");
		else {printf("UA was not received successfully!\n"); return -3;}

	}

	int ret=0;
	if(tcsetattr(fd,TCSANOW,&oldtio) != 0)
		ret += 2;
	if(close(fd) != 0)
		ret += 1;
	return ret;
}
