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

    char * buffer = malloc(567);
	
	char buf[255];
	buf[0] = 0x7e;
	int flag = -1;
	int i = 1;
	while(1){
		char res = receiveMessage(fd);
		
		if (res == 0x7e && flag == -1){
	 		flag = 0;
		}
		else if (res == 0x7e && flag == 0){
			continue;
		}
		else if (res != 0x7e && flag >= 0){
			buf[i++] = res;
			flag = 1;
		}
		else if (flag == 1 && res == 0x7e){
			buf[i] = 0x7E;
			break;
		}
	}

    llread(fd, buf);
	llread(fd, buf);


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
