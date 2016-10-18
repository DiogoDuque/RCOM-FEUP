#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

void printHex(char* hexMsg) {
	char hexArray[255];
	strcpy(hexArray,hexMsg);
	printf("HEX ARRAY(%lu):",strlen(hexArray));

	int i;
	for(i=0; i<strlen(hexMsg); i++) {
		printf(" 0x%02X",hexArray[i]);
	}
	printf("\n\n");
}


