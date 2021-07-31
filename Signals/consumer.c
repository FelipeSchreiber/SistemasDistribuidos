#include<stdio.h>
#include<signal.h>
#include<unistd.h>

void sig_handler(int signum)
{
  //Return type of the handler function should be void
	printf("\nInside handler function - %d Caught - ENDING EXECUTION\n",signum);
	raise(SIGINT);
}

void sig_handler2(int signum)
{
	printf("\nInside handler2\n");
}

void sig_handler3(int signum)
{
	printf("\nInside handler3\n");
}

int main(int argc,char *argv[]){
  signal(10,sig_handler); 
  signal(20,sig_handler2);
  signal(30,sig_handler3);
  printf("\nModo de uso:\n Passe o argumento 'b' para executar com busy wait, por default será executado sem busy wait\n");
  printf("\nThis process id is: %d\n",getpid());
  if(argc > 1 && argv[1][0] == 'b')
  {	
	  while(1)
	  {
	    printf("Inside main function\n");
	    sleep(1); 
	  }
  }
  else
  {
	  while(1)
	  {
	    printf("Inside main function\n");	
	    pause();
	  }
  }
 return 0; 
}


