#include <time.h>
#include <stdlib.h>
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
const unsigned char RR0= 0x05;
const unsigned char RR1= 0x85;
#define REJ0    0x01
#define REJ1    0x81


int mode;
int Nr = 1;
int flags; //for nonblock write

/**
 * Handles all the initialization code related to opening the serial port.
 * @param port port to be used for sending and receiving tramas.
 * @return file descriptor to read/write from/to the serial port.
 */
int init(char* port) {
	int seed = time(NULL);
    srand(seed);

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

    printf("\n\nNew termios structure set\n");
	return fd;
}

/**
 * Reads a byte from the fd.
 * @fd file descriptor used to read the trama.
 * @return byte read. if alarmFlag is on, returns 0 instead.
 */
unsigned char receiveMessage(int fd) {
	unsigned char buf[16];
    int res;
	while((res = read(fd,buf,1)) <= 0)
		if(alarmFlag)
			return 0x00;

	return buf[0];
}

/**
 * Attempts to send a trama.
 * @param fd file descriptor used to send the trama.
 * @param msg trama to send.
 * @param size size of the trama.
 * @return TRUE if trama was properly sent; FALSE otherwise.
 */
int sendMessage(int fd, unsigned char* msg, int size) {
	char tmp = msg[size/2];
	if(generateError != 0) { //generate possible error
		printf("Sending message with errors!\n");
		generateError = 0;
		msg[size/2] = 0x00;
	}
    int res = write(fd,msg,size);
	msg[size/2] = tmp;
	if(res==size) return TRUE;
	else return FALSE;
}

/**
 * Attempts to send a SET.
 * @param fd file descriptor used to send the trama.
 */
void sendSET(int fd){ //cmd
	unsigned char msg[5];
	msg[0]=FLAG; //F
	msg[1]=0x03; //A
	msg[2]=0x03; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F

	if(!sendMessage(fd,msg,5)==TRUE)
		printf("SET sent successfully!\n");
	printf("Warning: SET was not sent successfully!\n");
}

/**
 * Attempts to send an UA.
 * @param fd file descriptor used to send the trama.
 */
void sendUA(int fd){ //ans
	unsigned char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x01:0x03; //A
	msg[2]=0x07; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	
	if(sendMessage(fd,msg,5)==TRUE)
		printf("UA sent successfully!\n");
	else printf("Warning: UA was not sent successfully!\n");
}

/**
 * Attempts to send a DISC.
 * @param fd file descriptor used to send the trama.
 */
void sendDISC(int fd){ //cmd
	unsigned char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=0x0B; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	
	if(sendMessage(fd,msg,5)==TRUE)
		printf("DISC sent successfully!\n");
	else printf("Warning: DISC was not sent successfully!\n");
}

/**
 * Attempts to send a RR.
 * @param fd file descriptor used to send the trama.
 */
void sendRR(int fd){
    unsigned char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=Nr==0?0x05:0x85; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	
	if(sendMessage(fd,msg,5)==TRUE)
		printf("RR(%d) sent successfully!\n", Nr);
	else printf("Warning: RR was not sent successfully!\n");
}

/**
 * Attempts to send a REJ.
 * @param fd file descriptor used to send the trama.
 */
void sendREJ(int fd){
    unsigned char msg[5];
	msg[0]=FLAG; //F
	msg[1]=mode==TRANSMITTER?0x03:0x01; //A
	msg[2]=Nr==0?REJ0:REJ1; //C
	msg[3]=msg[1]^msg[2]; //BCC1
	msg[4]=FLAG; //F
	
	if(sendMessage(fd,msg,5)==TRUE) {
		printf("REJ sent successfully!\n");
		cntRejSend++;
	}
	else printf("Warning: REJ was not sent successfully!\n");
}

/**
 * State machine that tries to read a SET.
 * @param fd file descriptor used to send the trama.
 * @return TRUE if there was no problem with the SET; FALSE otherwise.
 */
int stateMachineSET(int fd) {
	unsigned char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		unsigned char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);
			//printf("received 0x%02X\n",info);
		}

		switch(st) {

		case START:
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
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
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			printf("RECEIVING SET: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING SET: length is incorrect. expected 5, was %d\n",counter);
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

/**
 * State machine that tries to read a UA.
 * @param fd file descriptor used to send the trama.
 * @return TRUE if there was no problem with the UA; FALSE otherwise.
 */
int stateMachineUA(int fd) {
	unsigned char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		unsigned char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);
			if(alarmFlag) return FALSE;
			}
			//printf("received 0x%02X\n",info);

		switch(st) {

		case START:
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
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
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			printf("RECEIVING UA: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING UA: length is incorrect. expected 5, was %d\n",counter);
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

/**
 * State machine that tries to read a REJ or RR.
 * @param fd file descriptor used to send the trama.
 * @return -2 if alarmFlag was on, -1 if the trama had some sort of error, 0 if REJ(0), 1 if REJ(1), 2 if RR(0), 3 if RR(1).
 */
int stateMachineR(int fd) {
	unsigned char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		unsigned char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);
			 //printf("received 0x%02X\n",info);
			if(alarmFlag) return -2;
		}

		switch(st) {

		case START:
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
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
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else
				st=LAST_F;
			break;

		case LAST_F:
			printf("RECEIVING R: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING R: length is incorrect. expected 5, was %d\n",counter);
				return -1;
			}

			if(msg[3] != (msg[1]^msg[2])) { //ANALYZE BCC1
				printf("RECEIVING R: BCC1 != (A XOR C)\n");
				return -1;
			}

			if(msg[1]=!0x03) {
				printf("RECEIVING R: A is incorrect. Expected 0x%02X, was 0x%02X\n",0x03,msg[1]);
				return -1;
			}

			if(msg[2]==0x01){
				printf("RECEIVING R: received REJ(0)\n");
				cntRejReceived++;
				return 0;
			} //ANALYZE C

			else if(msg[2]==0x81){
				printf("RECEIVING R: received REJ(1)\n");
				cntRejReceived++;
				return 1;
			}
			else if(msg[2]==0x05){
				printf("RECEIVING R: received RR(0)\n");
				return 2;
			}
			else if(msg[2]==0x85){
				printf("RECEIVING R: received RR(1)\n");
				return 3;
			}
			else {
				printf("RECEIVING R: A is incorrect.\n");
				return -1;
			}

		default:
			printf("RECEIVING R: NO STATE FOUND\n");
			return -3;
		}
	}
}

/**
 * State machine that tries to read a DISC.
 * @param fd file descriptor used to send the trama.
 * @return TRUE if there was no problem with the DISC; FALSE otherwise.
 */
int stateMachineDISC(int fd) {
	unsigned char msg[255];
	state st = START;
	int counter=0;

	while(TRUE){
		unsigned char info;
		if(st!=LAST_F) {
		 	info=receiveMessage(fd);
			if(alarmFlag) return FALSE;
			}
			//printf("received 0x%02X\n",info);

		switch(st) {

		case START:
			if(info!=0x7E)
				st=START;
			else st=FIRST_F;
			break;

		case FIRST_F:
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
			msg[counter]=info;
			counter++;
			if(info!=0x7E)
				st=READING;
			else st=LAST_F;
			break;

		case LAST_F:
			printf("RECEIVING DISC: ");
			printHex(msg,counter);

			if(counter != 5) { //ANALYZE STRLEN
				printf("RECEIVING DISC: length is incorrect. expected 5, was %d\n",counter);
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

/**
 * Starts the communication between the Transmitter and the Receiver.
 * @param port port to be used for sending and receiving tramas.
 * @param flag indicates if the caller is the Transmitter or the Receiver.
 * @return -2 if flag was wrong, -1 if didn't receive the trama successfully, else returns the file descriptor to read/write from/to the serial port.
 */
int llopen(char* port, int flag) {
	mode = flag;
	int fd = init(port);
	flags=fcntl(fd, F_GETFL, 0); //read is in nonblock so
	fcntl(fd, F_SETFL, flags | O_NONBLOCK); //alarm works
	if(mode == RECEIVER) {
		if(stateMachineSET(fd)==TRUE) {
			printf("SET received successfully!\n");
			sendUA(fd);
			return fd;
		} else {
			printf("SET was not received successfully\n");
			close(fd);
			return -1;
		}
	} else if(mode == TRANSMITTER) {
		sendSET(fd);
		if(stateMachineUA(fd)==TRUE) {
			printf("UA received successfully!\n");
			alarmFlag=0;
			return fd;
		} else {
			printf("UA was not received successfully\n");
			close(fd);
			return -1;
		}
	} else return -2;
}

/**
 * Writes buffer to fd.
 * @param fd file descriptor.
 * @param buffer trama to send.
 * @param length trama length.
 * @return TRUE if trama was successfully sent, FALSE otherwise.
 */
int llwrite(int fd, unsigned char* buffer, int length) {
	int size = length + 5;
	unsigned char package [2*length + 5];
	unsigned char A = 0x03;
	static unsigned char C = 0x00;

	int i;
	int offset = 4;
	for (i = 0; i < length; i++) {
		unsigned char byte = buffer[i];

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
	package[size-1] = FLAG;

	int res=FALSE;
	if (randInterval(0, 10) < 1)
		generateError = 1;
	
    while(alarmCounter < retries) {
        if(alarmFlag) {
            alarm(timeOut);
            alarmFlag=0;
			
			if (alarmCounter == 0)
				cntTrasmit++;
			else cntRetransmit++;
			res=sendMessage(fd, package,size);
			if (!res) continue;

			int resMachine;
			if((resMachine=stateMachineR(fd))>=2) {
				if(C==0x40 && resMachine==2) 
					C = INF0;
				else if (C==0x00 && resMachine==3)
					C = INF1; 
				else {
					C = C^0x40;
					continue;
				}
				break;
			}
			res=FALSE;
        }
    }

    alarm(0);
	alarmFlag=1;
	alarmCounter=0;
	return res;
}

/**
 * Calculates expected bcc value from given buffer
 * @param buf        buffer of bytes on which bcc will be calculated
 * @param bufLength  length of buf array
 * @return bcc's expected value
 */
unsigned char calcBCC(unsigned char * buf, int bufLength){
    int i = 0;
    unsigned char res = 0x00;
    for (i = 0; i < bufLength; i++){
        res ^= buf[i];
    }
    return res;
}

struct Trama {
    unsigned char address;
    unsigned char control;
    unsigned char bcc1;
    unsigned char data[2048];
    int dataLength;
    unsigned char bcc2;
};

/**
 * reads fd's content and loads trama's fields accordingly.
 * @param fd     file descriptor.
 * @param trama  empty Trama to fill with information from fd.
 * @return trama's size, in bytes.
 */
int readTrama(int fd, struct Trama * trama){
    int nrBytes = 0;
    int r;
    unsigned char c, res;
    int delta = 4;

    unsigned char buf[2048];
	buf[0] = 0x7e;
	int flag = -1;
	int i = 1;

	while(1){
		
		res = receiveMessage(fd);
		if(alarmFlag)
			return -2;
		
		if (res == FLAG && flag == -1){
	 		flag = 0;
		}
		else if (res == FLAG && flag == 0){
			continue;
		}
		else if (res != FLAG && flag >= 0){
			buf[i++] = res;
			flag = 1;
		}
		else if (flag == 1 && res == FLAG){
			buf[i] = FLAG;
			break;
		}
	}
	

    //filling header
    trama->address = buf[1];
    trama->control = buf[2];
    trama->bcc1 =    buf[3];
	
    //data field loop

    if (trama->control == INF0 || trama->control == INF1){
        i = 0;
        while (TRUE){
            c = buf[i+delta];

            if (c == FLAG){
            	if (i != 0){
            		trama->bcc2 = trama->data[i-1];
		            trama->data[i-1] = 0;
                    trama->dataLength = i-1;

		            return trama->dataLength + delta + 1; //real trama length is i+escape_offset-1+6 = i+5
            	} else {
					trama->bcc2 = 0x00;
		            trama->data[0] = 0;
                    trama->dataLength = 0;

		            return 5;
				}
            }

            //destuffing
            else if (c == ESCAPE){
                delta++;
                c = buf[i+delta];

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

    unsigned char str[18];
    sprintf(str, "error on read (i=%d)\n", i);

    perror(str);
    return -1;
}

/**
 * Reads a trama from the fd.
 * @param fd file descriptor.
 * @param buffer where to read trama.
 * @return -1 if there was an error on the trama, 0 if found a duplicated, else returns the trama length.
 */
int llread (int fd, unsigned char * buffer) {
    struct Trama trama;
    int i;

    while(TRUE) {
        int tramaLength = readTrama(fd, &trama);
		
        if (tramaLength < 5){   //least amount of memory a trama will need
            perror("Error on readTrama\n");
            return -1;  //Tramas I, S ou U com cabecalho errado sao ignoradas, sem qualquer accao (1)
        } else if (tramaLength == -1)
			return -1;

        if (trama.address^trama.control == trama.bcc1){    //i.e. if header is correct
            //llread does will never receive tramas of type RR or REJ
            switch(trama.control){
                case INF0:  //Ns = 0
                    if (Nr == 1){   //data is not duplicate
                        if (calcBCC(trama.data, trama.dataLength) == trama.bcc2){   //data bcc is correct
                            //accept trama
							printf("BCC TRUE ");

                            for (i = 0; i < trama.dataLength; i++){
                                buffer[i] = trama.data[i];
                            }

                            sendRR(fd);
                            Nr = 0;
							cntReceived++;
                            return tramaLength;
                        }
                        else {  //bcc is incorrect
						printf("BCC FALSE ");
                            sendREJ(fd);
                            return -1;
                        }
                    }
                    else {  //duplicate data
						printf("REPEATED");
                        sendRR(fd);
                        return 0;
                    }
                    break;
                
                case INF1:  //Ns = 1
                    if (Nr == 0){   //data is not duplicate
                        if (calcBCC(trama.data, trama.dataLength) == trama.bcc2){   //data bcc is correct
                            //accept trama
                            for (i = 0; i < trama.dataLength; i++){
                                buffer[i] = trama.data[i];
                            }

                            sendRR(fd);
                            Nr = 1;
							cntReceived++;
                            return tramaLength;
                        }
                        else {  //data is incorrect
                            sendREJ(fd);
                            return -1;
                        }
                    }
                    else {  //duplicate data
                        sendRR(fd);
                        return 0;
                    }
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

/**
 * Attempts to finish communications between the Transmitter and the Receiver.
 * @param fd file descriptor.
 * @return 0 if everything went right, -1 if Receiver didn't receive DISC, -2 if Transmitter didn't receive DISC, -3 if Receiver didn't receive UA, 1 if there was an error closing fd, 2 if there was an error calling tcsetattr, 3 if the last two results happened simultaneously.
 */
int llclose(int fd) {
	printf("Starting to close...\n\n");

	if(mode==TRANSMITTER) { /* ---------MODE TRANSMITTER--------- */


		//send & receive DISC
		int resDISC;
		alarmFlag=1;
		while(alarmCounter < retries) {
        	if(alarmFlag) {
        		alarm(timeOut);
    	    	alarmFlag=0;
    	    	sendDISC(fd);
				resDISC=stateMachineDISC(fd);
				if(resDISC==TRUE) break;
      		}
    	}
    	alarm(0);
		alarmCounter=0;
		alarmFlag=0;
		if(resDISC==TRUE)printf("DISC received successfully!\n");
		else {printf("DISC was not received successfully!\n"); return -2;}

		//send UA
		sleep(1);
		sendUA(fd);

	} else { /* -------------MODE RECEIVER------------- */

		//receive DISC
		alarmCounter=0;
		alarmFlag=1;
		int resDISC;
		while(alarmCounter < retries) {
        	if(alarmFlag) {
        		alarm(timeOut);
    	    	alarmFlag=0;
				resDISC=stateMachineDISC(fd);
				if(resDISC==TRUE) break;
      		}
    	}
    	alarm(0);
		alarmCounter=0;
		alarmFlag=0;
		if(resDISC==TRUE)printf("DISC received successfully!\n");
		else {printf("DISC was not received successfully!\n"); return -1;}

		//send DISC & receive UA
		alarmCounter=0;
		alarmFlag=1;
		int resUA;
		while(alarmCounter < retries) {
        	if(alarmFlag) {
        		alarm(timeOut);
    	    	alarmFlag=0;
				sendDISC(fd);
				resUA=stateMachineUA(fd);
				if(resUA==TRUE) break;
      		}
    	}
    	alarm(0);
		alarmCounter=0;
		alarmFlag=0;
		if(resUA==TRUE) printf("UA received successfully!\n");
		else {printf("UA was not received successfully!\n"); return -3;}

	}

	fcntl(fd, F_SETFL, flags);

	int ret=0;
	if(tcsetattr(fd,TCSANOW,&oldtio) != 0)
		ret += 2;
	if(close(fd) != 0)
		ret += 1;
	return ret;
}
