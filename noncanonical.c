/*Non-Canonical Input Processing*/
#include "utils.h"
#include "dataLayer.h"
#include <time.h>
#include <stdlib.h>

struct at_control {
    char control;
    char t1;
    int l1;
    int fileSize;
    char t2;
    int l2;
    char fileName[2048];
};

int readControl(char * buffer, struct at_control * sf_control){

    sf_control->control = buffer[0];
    sf_control->t1 = buffer[1];

    char c[4];
    c[0] = buffer[2];
    c[1] = 0;
    c[2] = 0;
    c[3] = 0;
    sf_control->l1 = *(int *) c;

    char len[4];
    int i;
    for (i = 0; i < sf_control->l1; i++){
        len[i] = buffer[3+i];
    }
    sf_control->fileSize = *(int *) len;    //tested: works

    sf_control->t2 = buffer[3+sf_control->l1];  //7

    char d[4];
    d[0] = buffer[4 + sf_control->l1];    //8
    d[1] = 0;
    d[2] = 0;
    d[3] = 0;
    sf_control->l2 = *(int *) d;

    for(i = 0; i < sf_control->l2; i++){    //i seria = 9
        sf_control->fileName[i] = buffer[5 + sf_control->l1 + i];
    }
    sf_control->fileName[sf_control->l2] = 0;

    return 5 + sf_control->l1 + sf_control->l2;
}

struct at_data {
    char control;
    int n;
    int k;
    char data[2048];
};

int readData(char * buffer, struct at_data * data){

    data->control = buffer[0];

    char c[4];
    c[0] = buffer[1];
    c[1] = 0;
    c[2] = 0;
    c[3] = 0;
    data->n = *(int *) c;

    int i, l1, l2;

    char d[4];
    d[0] = buffer[2];
    d[1] = 0;
    d[2] = 0;
    d[3] = 0;
    l2 = *(int *) d;

    char e[4];
    e[0] = buffer[3];
    e[1] = 0;
    e[2] = 0;
    e[3] = 0;
    l1 = *(int *) e;

    data->k = 256 * l2 + l1;

    for (i = 0; i < data->k; i++){
        data->data[i] = buffer[i + 4];
    }

    return 4 + data->k;
}

void printStatus(int code) {
	printf("\n\n-----> STATUS <-----\n");
	printf("Packages received: %d\n", cntReceived);
	printf("Number of time-outs: %d\n", cntTimeOuts);
	printf("REJ sent: %d\n", cntRejSend);

	exit(code);
}

int main(int argc, char** argv){
    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort [MaxPackageSize (1024 MAX)] [Retries] [TimeOut]\n");
		printf("\tex: nserial /dev/ttyS1 [512] [3] [3]\n");
        exit(1);
    } else if (argc == 5) {
		maxPackageSize = (int) strtol(argv[2], NULL, 10);
        maxPackageSize = maxPackageSize > 1024 ? 1024 : maxPackageSize;
		retries = (int) strtol(argv[3], NULL, 10);
		timeOut = (int) strtol(argv[4], NULL, 10);
	} 

	(void) signal(SIGALRM, atende); // Instala a rotina que atende interrupcao

	int fd=llopen(argv[1], RECEIVER);
	if(fd < 0) {
		printf("Error with llopen\n");
        return -1;
	}
	
    int n = 0, fileSize = 0, res;
    FILE * f1;
    

    while (1){
		char buffer[2048];
		alarmCounter=0;
		alarmFlag=1;
		while(alarmCounter < retries) {
        	if(alarmFlag) {
        		alarm(timeOut);
    	    	alarmFlag=0;
				res = llread(fd, buffer);
				if(res>-1) break;
      		}
    	}
    	alarm(0);
		alarmFlag=0;

        if (res > 0){
            
            if (buffer[0] == 0x02 || buffer[0] == 0x03){    //control data
                struct at_control sf_control;
                readControl(buffer, &sf_control);
                
                if (sf_control.control == 0x02){
                    //create file with name: sf_control.filename
                    printf("---->%s\n", sf_control.fileName);
                    f1 = fopen(sf_control.fileName, "a+");   //might change to open(3)
                    //f1 = fopen("pguin.gif", "w");   //might change to open(3)
                    //fclose(f1);
                }
                else if (sf_control.control == 0x03){
                    //close file with name sf_control.filename
                    int fsize;
                    fseek(f1, 0, SEEK_END);
                    fsize = ftell(f1);
                    fseek(f1, 0, SEEK_SET);
                    
                    if (fsize == fileSize){
                        printf("File Received Correctly\n");
                    }
                    else {
                        perror("File Received With Errors\n");
                    }

                    fclose(f1);
					break;
                }
            }
            else if (buffer[0] == 0x01){
                //data packet
                struct at_data data;
                readData(buffer, &data);

                if (data.n == (n)){
                    printHex(data.data, data.k);
                    fwrite(data.data, 1, data.k, f1);
                    printf("\nRead package #%d\n", n);
                    n++;
                    fileSize += data.k;
                }
            }
        }
        else if (res == 0){
            printf("repetido\n");
        }
        else if (res == -1){
            printf("dados incorrectos\n");
			if(alarmCounter >= retries) {
				alarm(0);
				alarmCounter = 0;
				break;
			}
        }
		else if (res==-2){
			printf("ALARM TIMEOUT\n");
			break;
		}
    }


	//llread(fd, buffer);

    //printf("AFTER LLREAD: %s\n", buffer);


    switch(llclose(fd)){
	case 0:
		printf("Closed receiver successfully!\n");
		printStatus(0);

	case 1:
		printf("Error closing file descriptor...\n");
		printStatus(1);

	case 2:
		printf("Error with tcsetattr()...\n");
		printStatus(2);

	case 3:
		printf("Error closing file descriptor and with tcsetattr()...\n");
		printStatus(3);

	default:
		printStatus(-1);
	}
}
