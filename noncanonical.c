/*Non-Canonical Input Processing*/
#include "utils.h"
#include "dataLayer.h"

int main(int argc, char** argv){
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	int fd=llopen(argv[1], RECEIVER);
	if(fd < 0) {
		printf("Error with llopen\n");
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
}
