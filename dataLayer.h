#include <unistd.h>

char receiveMessage(int fd) {
	char buf[16];
	buf[0]='\0';
    int res = read(fd,buf,1);

   	//buf[res]='\0';          /* so we can printf... */

	int res2=write(fd,buf,1);

	return buf[0];
}

//tries to send a message. if was properly sent, returns TRUE; otherwise returns FALSE
int sendMessage(int fd, char* msg) {

	int size = strlen(msg);
    int res = write(fd,msg,size);
    //printf("writing: «%s»; written %d of %d written\n\n\n", msg,res,size);

	if(res==size) return TRUE;
	else return FALSE;


}

int llwrite(int fd, char* buffer, int length){

}

int llread (int fd, char* buffer) {

}
