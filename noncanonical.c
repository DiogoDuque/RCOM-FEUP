/*Non-Canonical Input Processing*/
#include "utils.h"
#include "dataLayer.h"

struct at_control {
    char control;
    char t1;
    int l1;
    int fileSize;
    char t2;
    int l2;
    char fileName[1024];
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
    char data[1024];
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
        return -1;
	}
	
    int n = 0, fileSize = 0, res;
    FILE * f1;
    char buffer[1024];

    while (1){
        res = llread(fd, buffer);

        if (res > 0){
            if (buffer[0] == 0x02 || buffer[0] == 0x03){    //control data
                struct at_control sf_control;
                readControl(buffer, &sf_control);

                if (sf_control.control == 0x02){
                    //create file with name: sf_control.filename
                    printf("---->%s\n", sf_control.fileName);
                    //f1 = fopen(sf_control.fileName, "a+");   //might change to open(3)
                    f1 = fopen("pguin.gif", "a+");   //might change to open(3)
                    //f1 = fopen("test.txt", "a+");   //might change to open(3)
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

                    char eos = '\0';
                    fprintf(f1, "%c", eos);
                    fclose(f1);
                    break;
                }
            }
            else if (buffer[0] == 0x01){
                //data packet
                struct at_data data;
                readData(buffer, &data);

                if (data.n == (n + 1)){
                    printHex(data.data, data.k);
                    fprintf(f1, "%s", data.data);
                    n++;
                    fileSize += data.k;
                }
            }
        }
    }


	//llread(fd, buffer);

    //printf("AFTER LLREAD: %s\n", buffer);


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
