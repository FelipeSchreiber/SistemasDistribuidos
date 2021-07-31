#include<stdio.h>
#include <stdlib.h>
#include<signal.h>
#include<unistd.h>
#include <errno.h>
void error(const char *msg)
{
    perror(msg);
    exit(2);
}

int main(){
	pid_t pid;
	int signalToSend;
	printf("Insert the process id\n");
	scanf("%d",&pid);
	printf("Insert the signal to be sent\n");
	scanf("%d",&signalToSend);
	kill(pid,signalToSend);
	switch(errno){
		case EINVAL:
			error("ERROR Invalid signal");
			break;
		case EPERM:
			error("ERROR No permission to transmit");
			break;
		case ESRCH:
			error("ERROR Invalid process");
			break;
	}
	return 0;
}
