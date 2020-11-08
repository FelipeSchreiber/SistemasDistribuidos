#include <stdio.h> 
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h>
#include <sys/wait.h>

#define PORT 8080 

char* getTime()
{
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  return asctime (timeinfo);
}

void writeFile(char* myPid,int k)
{
	FILE *myFile = fopen("resultado.txt", "a" );
	char *curTime = getTime();
	fprintf(myFile, "ProcessID: %sTime: %s",myPid,curTime);
	dprintf(1,"Waiting %d secs\n",k);
	sleep(k);
	fclose(myFile);
}

int Connect()
{
	int sock = 0; 
	struct sockaddr_in serv_addr;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 
	
	while((connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)) 
	{ 
		printf("\nConnection Failed \n");
	}
	return sock;
}

int request(char*buff,char*str_PID,int sock)
{
	int valread;
	strncpy(buff, "1|\0", 1024);
	strcat(buff,str_PID);
	dprintf(1,"Enviando mensagem %s\n",buff);
	send(sock , buff , 1024 , 0 ); 
	valread = read(sock , buff, 1024); 
	if(valread == 0 )
	{	
		dprintf(1,"A PROBLEM OCCURRED\n");
	}
	//printf("%s\n",buff );
	if (strcmp(buff,"2") == 0)
	{
		dprintf(1,"GOT GRANT\n");
		return 1;		
	}
	return 0;
}

void release(char *buff,char*str_PID,int sock)
{
	strncpy(buff, "3|\0",1024);
	strcat(buff,str_PID);
	dprintf(1,"Enviando mensagem %s\n",buff);
	send(sock , buff , 1024 , 0 );
	dprintf(1,"RELEASE GRANT\n"); 	
}

void makeRRequests(int r,char*buff,char*str_PID,int sock,int k)
{
	int ok;
	for(int j=0;j<r;j++)	
	{
		ok = request(buff,str_PID,sock);
		if(ok)
		{
			writeFile(str_PID,k);
			release(buff,str_PID,sock);
		}
	}
	close(sock);
}

int main(int argc, char const *argv[]) 
{ 
	int r = atoi(argv[1]);
  	int k = atoi(argv[2]);
  	int n = atoi(argv[3]) -1;
  	int *pids = malloc(n*sizeof(int));
	for (int i = 0; i < n; i++)
  	{
    		if ((pids[i] = fork()) < 0)
    		{
    			exit(1);
    		}
    		//if((pids[i]==0))
    		else 
    		{
    			int process_id = getpid();
			char str_pid[1024];
			char buffer[1024] = {0};
			sprintf(str_pid,"%d|",process_id); 
			int sock = Connect();
			makeRRequests(r,buffer,str_pid,sock,k);
    		}
    	}
    	int status;
	int pid;
	while (n > 0)
	{
		pid = wait(&status);
		--n;
	}
	return 0; 
} 


