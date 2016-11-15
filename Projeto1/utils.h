#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

int alarmFlag=0, alarmCounter=0;
int maxPackageSize = 512, retries = 3, timeOut = 3;
int cntTrasmit = 0, cntRetransmit = 0, cntReceived = 0, cntRejSend = 0, cntRejReceived = 0, cntTimeOuts = 0;

int generateError = 0;
unsigned int randInterval(unsigned int min, unsigned int max)
{
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

void atende() {
	cntTimeOuts++;
    alarmCounter++;
    printf("alarme # %d\n", alarmCounter);
    alarmFlag=1;
}

void printHex(unsigned char* hexMsg, int size) {
	printf("HEX ARRAY:");

	int i;
	for(i=0; i<size; i++) {
		printf(" 0x%02X",hexMsg[i]);
	}
	printf("\n");
}


