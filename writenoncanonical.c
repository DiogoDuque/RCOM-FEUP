/*Non-Canonical Input Processing*/
#include <signal.h>
#include "aux.h"
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
	int fd = init(argv[1]);
	char msg[16];
	msg[0]=0x7E;
	msg[1]=0x03;
	msg[2]=0x03;
	msg[3]=0x00;
	msg[4]=0x7E;

    while(conta < 4) {
        if(flag) {
			sendMessage(fd,msg);
            alarm(3);
            flag=0;
            printf("Foi chamado ahahah :D");
        }
    }
    alarm(0);
    printf("Count: %d", conta);
    //llopen();
    //sleep(3);

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
