/*Non-Canonical Input Processing*/
#include <signal.h>
#include "utils.h"
#include "dataLayer.h"

#define MODEMDEVICE "/dev/ttyS1"

int flag=1, conta=1;

void atende() {
    printf("alarme # %d\n", conta);
    flag=1;
    conta++;
}

int main(int argc, char** argv) {    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }
    
    (void) signal(SIGALRM, atende); // Instala a rotina que atende interrupcao
	int fd=-1;

    while(conta < 4) {
        if(flag) {
			fd=llopen(argv[1], TRANSMITTER);
            alarm(3);
            flag=0;
        }
    }
    alarm(0);
    printf("Count: %d", conta);
    //sleep(3);

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
}
