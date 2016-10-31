/*Non-Canonical Input Processing*/
#include "utils.h"
#include "dataLayer.h"

#define MODEMDEVICE "/dev/ttyS1"

int send(int fd, char* buffer, int size) {
	char package [size + 5];

	char C = 0x01;
	static char N = 0x00;
	char L2 = size/256;
	char L1 = size%256;
	char BCC2 = 0x00;

	package[0] = C;
	package[1] = N;
	package[2] = L2;
	package[3] = L1;

	int i;
	for (i = 0; i < size; i++) 
		package[i + 4] = buffer[i];
	for (i = 0; i < size + 4; i++)
		BCC2 = BCC2^package[i];
	package[i] = BCC2;

	N++;
	return llwrite(fd, package, size + 5);
}

int sendStart(int fd, char* fileSize, char* fileName) {
	int size = 10 + strlen(fileName);
	char package[size];

	char C = 0x02;
	char T1 = 0x00;
	char L1 = 0x04; // fileSize is an array with 4 hex values
	char T2 = 0x01;
	char L2 = strlen(fileName);
	char BCC2 = 0x00;

	package[0] = C;
	package[1] = T1;
	package[2] = L1;
	package[3] = fileSize[0];
	package[4] = fileSize[1];
	package[5] = fileSize[2];
	package[6] = fileSize[3];
	package[7] = T2;
	package[8] = L2;

	int i;
	for (i = 0; i < strlen(fileName); i++)
		package[i+9] = fileName[i];
	for (i = 0; i < size -1; i++)
		BCC2 = BCC2^package[i];
	package[i] = BCC2;

	return llwrite(fd, package, size);
}

int sendEnd(int fd, char* fileSize, char* fileName) {
	int size = 10 + strlen(fileName);
	char package[size];

	char C = 0x03;
	char T1 = 0x00;
	char L1 = 0x04; // fileSize is an array with 4 hex values
	char T2 = 0x01;
	char L2 = strlen(fileName);
	char BCC2 = 0x00;

	package[0] = C;
	package[1] = T1;
	package[2] = L1;
	package[3] = fileSize[0];
	package[4] = fileSize[1];
	package[5] = fileSize[2];
	package[6] = fileSize[3];
	package[7] = T2;
	package[8] = L2;

	int i;
	for (i = 0; i < strlen(fileName); i++)
		package[i+9] = fileName[i];
	for (i = 0; i < size -1; i++)
		BCC2 = BCC2^package[i];
	package[i] = BCC2;

	return llwrite(fd, package, size);
}

int sendFile(int fd, char* fileName) {
	printf("\n\n-----> SENDING FILE ");
	printf(fileName);
	printf(" <-----\n");

	FILE * f1 = fopen(fileName, "r");
    if (!f1) return -1;

	int fsize;
    fseek(f1, 0, SEEK_END);
    fsize = ftell(f1);
    fseek(f1, 0, SEEK_SET);

    char fileSize[4];
    fileSize[3] = (fsize >> 24) & 0xFF;
    fileSize[2] = (fsize >> 16) & 0xFF;
    fileSize[1] = (fsize >> 8) & 0xFF;
    fileSize[0] = fsize & 0xFF;

	printf("\n\n-----> SENDING START PACKAGE <-----\n");
    if (!sendStart(fd, fileSize, fileName)) { fclose(f1); return -2; }
	
	int maxPackageSize = 512;
	int c;
	int size = 0;
	int counter = 0;
	char package[maxPackageSize];
	
	while((c = getc(f1)) != EOF) {
		package[size++] = c;

		if (size == maxPackageSize) {
			printf("\n\n-----> SENDING FILE PACKAGE #%d - %d Bytes <-----\n", counter++, size);
			if (!send(fd, package, size)) { fclose(f1); return -3; }
			size = 0;
		}
	}
	fclose(f1);
	printf("\n\n-----> SENDING FILE PACKAGE #%d - %d Bytes <-----\n", counter++, size);
	if (!send(fd, package, size)) return -3;

	printf("\n\n-----> SENDING END PACKAGE <-----\n");
	if (!sendEnd(fd, fileSize, fileName)) return -4;
}


int main(int argc, char** argv) {
	char * fileName = "pinguim.gif";

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    } else if (argc == 3) {
		fileName = argv[2];
	}

    (void) signal(SIGALRM, atende); // Instala a rotina que atende interrupcao
	int fd=-1;
	alarmFlag=1;

    while(alarmCounter < MAX_RETRANSMISSIONS) {
        if(alarmFlag) {
        alarm(3);
        alarmFlag=0;
		fd=llopen(argv[1], TRANSMITTER);
		if(fd>0) break;
      }
    }
    alarm(0);
	alarmCounter=0;
	alarmFlag=1;
	if (fd <= 0)
		return -1;

	switch(sendFile(fd, "pinguim.gif")) {
		case 0:
			printf("File sent successfully!\n");
			break;
		case -1:
			printf("Couldn't open file!\n");
			break;
		case -2:
			printf("Error sending start package!\n");
			break;
		case -3:
			printf("Error sending file package!\n");
			break;
		case -4:
			printf("Error sending end package!\n");
			break;
	} 

	switch(llclose(fd)){
	case 0:
		printf("Closed receiver successfully!\n");
		return 0;

	case 1:
		printf("Error closing file descriptor...\n");
		return 1;

	case 2:
		printf("Error with tcsetattr()...\n");
		return 2;

	case 3:
		printf("Error closing file descriptor and with tcsetattr()...\n");
		return 3;

	default:
		return -1;
	}

	return -1;
}
