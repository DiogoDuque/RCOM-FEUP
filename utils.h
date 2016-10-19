#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

int alarmFlag=0, alarmCounter=0;

void atende() {
    alarmCounter++;
    printf("alarme # %d\n", alarmCounter);
    alarmFlag=1;
}

void printHex(char* hexMsg, int size) {
	printf("HEX ARRAY:");

	int i;
	for(i=0; i<size; i++) {
		printf(" 0x%02X",hexMsg[i]);
	}
	printf("\n\n");
}


