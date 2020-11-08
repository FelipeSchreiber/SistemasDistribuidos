#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <semaphore.h>
#include <errno.h> 
#include "FilaLista.h"
#define PORT 8080
#define DELIM "|" //define o caractere delimitador das mensagens
#define MAX_CLIENTS 128
unsigned long server_fd, new_socket, valread,max_sd,sd; 
struct sockaddr_in address; 
int opt = 1; 
int addrlen = sizeof(address); 
char buffer[1024] = {0};//buffer de leitura do socket
char *clientMessageParsed[3]; //vetor onde será armazenada a mensagem do cliente separada por DELIM
Atendimentos *contaAtendimento[MAX_CLIENTS];//vetor onde é armazenada uma estrutura com o id do processo e quantas vezes recebeu GRANT
char hello[1024] = "Hello from server";
unsigned long int clientWithGrant = 0;//variavel que guarda o socket que está com GRANT. Será usado pra recuperar caso um cliente caia com o GRANT 
sem_t sm_Queue;//prevent multiple threads access Queue at the same time
sem_t sm_ClientList;//prevent multiple threads access clients list at the same time
sem_t sm_AtendimentoList;
sem_t sm_noClientWithGrant;//prevents poping the message queue if there's a client with Grant
sem_t sm_MAIN;//Mutex that prevents Main executing before other threads finish
unsigned long int client_sockets[MAX_CLIENTS] = {0};
fd_set readfds;//Conjunto para ficar escutando caso um dos sockets clientes ou do servidor tenha mensagem
Fila *Q;//fila que armazena as requisições de Grant
int KEEP_HANDLING = 1;
int KEEP_READING_QUEUE = 1;
int KEEP_LISTENING = 1;
int activity = 0;

int checkThreadInClientList(int thrId)
{
	int i;
	for(i = 0;i<MAX_CLIENTS;i++)
	{
		if(thrId == client_sockets[i])
			return 1;
	}
	return 0;
}
//funcao para separar a mensagem recebida
void parse(char *buff)
{
	char *copy,*p;
	int cnt = 0;
	copy = strdup(buff);
	while(p = strsep(&copy,DELIM))
  	{	
  		clientMessageParsed[cnt] = strdup(p);
  		cnt++;
  	}
  	free(copy);
}

int getFreeSpaceAtendimentoList()
{
	int pos = -1;
	//sem_wait(&sm_AtendimentoList);
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		if (contaAtendimento[i]->id_processo == -1)
		{
			pos = i;
			break;
		}
	}
	//sem_post(&sm_AtendimentoList);
	return pos;
}

//inicializa todos os elementos do vetor contaAtendimento com uma estrutura default do tipo Atendimentos cujo id == -1 e count == 0
void initializeProcessCounter()
{
	int curr;
	for(curr = 0;curr<MAX_CLIENTS;curr++)
	{
		contaAtendimento[curr] = malloc(sizeof(Atendimentos));
		contaAtendimento[curr]->id_processo = -1;
		contaAtendimento[curr]->count=0;
	}
}

//incrementa a quantidade de vezes que um processo recebeu grant
void incrementProcessCounter(int pId)
{	
	Atendimentos *a;
	int achou = 0;
	int curr = 0;
	int livre = -1;
	sem_wait(&sm_AtendimentoList);
	while(curr<MAX_CLIENTS && !achou)
	{
		a = contaAtendimento[curr];
		if(a->id_processo == pId)
		{
			a->count++;
			achou = 1;
			//dprintf(1,"ContaAtendimento achada:%d\n",contaAtendimento[curr]->id_processo);
		}
		curr++;
	}
	if(!achou)
	{
		livre = getFreeSpaceAtendimentoList();
		if(livre!=-1)
		{
			contaAtendimento[livre]->id_processo = pId;
			contaAtendimento[livre]->count=1;
			//dprintf(1,"ContaAtendimento criada:%d\n",contaAtendimento[livre]->id_processo);
		}
	}
	sem_post(&sm_AtendimentoList);	
}

//funcao que manda o grant ao cliente 
void requestMessageReceived(Message *msg)
{
	//envia 2 que é o código do grant
	dprintf(1,"Executando request do processo %ld\n",msg->id);
	if(checkThreadInClientList(msg->fd))
	{
		strncpy(hello, "2", 1024);
		//dprintf(1,"Enviando msg %s para cliente\n",hello);
		dprintf(1,"Giving Grant to process %ld\n",msg->id);
		send(msg->fd , hello , 1024 , 0 );
		clientWithGrant = msg->fd;
	}
	else
	{
		sem_post(&sm_noClientWithGrant);
	}
}

//permite que um novo elemento seja lido da lista e reseta a variavel que guarda o processo com grant
void releaseMessageReceived()
{
	//dprintf(1,"\n\nRELEASE RECEIVED\n\n");
	sem_post(&sm_noClientWithGrant);
	clientWithGrant = 0;	
}

//pega lock clientlist
int checkEmptySocketList()
{
	int vazia = 1;
	sem_wait(&sm_ClientList);
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		if (client_sockets[i] != 0)
		{
			vazia = 0;
			break;
		}
	}
	sem_post(&sm_ClientList);
	return vazia;
}

//funcao que encerra o program quando o usuario digita "pare" no terminal
void processaPare()
{
	int is_empty = 0;
	is_empty = checkEmptySocketList();
	KEEP_HANDLING = 0;
	KEEP_LISTENING = 0;
	KEEP_READING_QUEUE = 0;
	//dprintf(1,"OK,%s\n",is_empty?"Socket List Vazio":"Socket List Não vazio");
	if(!is_empty)
	{
		sem_wait(&sm_ClientList);
		for(int i=0;i<MAX_CLIENTS;i++)
		{
			close(client_sockets[i]);
		}
		close(server_fd);
		sem_post(&sm_ClientList);
	}
}

//funcao que imprime na tela os pedidos pendentes quando o usuario digita "pedidos" no terminal
void processaPedidos()
{
	sem_wait(&sm_Queue);
	Lista *l = Q->ini;
	Message *msm;
	dprintf(1,"|Começo Lista PEDIDOS|\n=============================\n");
	while(l!=NULL)
	{
		msm = l->info;
		dprintf(1,"|processo: %ld|\n",msm->id);
		l = l->prox;
	}
	dprintf(1,"|Fim Lista PEDIDOS|\n=============================\n");
	sem_post(&sm_Queue);
}

//funcao que imprime na tela quantas vezes cada processo recebeu grant quando o usuario digita "atendimento" no terminal
void processaAtendimento()
{
	Atendimentos *a;
	sem_wait(&sm_AtendimentoList);
	int curr; 
	dprintf(1,"|Começo Lista Atendimento|\n=============================\n");
	for(curr = 0;curr<MAX_CLIENTS;curr++)
	{
		a = contaAtendimento[curr];
		if((a->id_processo != -1))
		{
		      dprintf(1,"|processo: %ld|servido: %d vezes\n",\
		      a->id_processo,a->count);							
		}	
	}
	dprintf(1,"|Fim Lista Atendimento|\n=============================\n");
	sem_post(&sm_AtendimentoList);
}

//pega lock clientelist
void *listenCLI()
{
	char input[1024]; 
	//STDIN 0
	//STDOUT 1
	//STDERR 2
	while(KEEP_LISTENING)
	{
		//dprintf(1,"%s","Inside LISTEN_CLI\n");
		read(0,input,1024);
		input[strlen(input) - 1] = '\0';
		if (strcmp(input,"empty") != 0)
		{
			//dprintf(1,"%s (%s)--EVAL:%d\n","Input is:",input,(strcmp(input,"stop") == 0));
			if (strcmp(input,"pare") == 0)
			{
				processaPare();				
			}
			if (strcmp(input,"pedidos") == 0)
			{
				processaPedidos();
			}
			if (strcmp(input,"atendimento") == 0)
			{
				processaAtendimento();
			}
			strncpy(input, "empty", sizeof(input));
		}
	}
}

//pega lock de cliente
int getFreeSpaceSocketList()
{
	int pos = -1;
	sem_wait(&sm_ClientList);
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		if (client_sockets[i] == 0)
		{
			pos = i;
			break;
		}
	}
	sem_post(&sm_ClientList);
	return pos;
}
//pega lock de client_list
int saveSocketList(unsigned long int socketId)
{
	int found = getFreeSpaceSocketList();
	if(found != -1)
	{
		sem_wait(&sm_ClientList);
		client_sockets[found] = socketId;
		sem_post(&sm_ClientList);
	}
	else
	{
		close(socketId);	
	}
	return found;
}
//pega lock de clientes
void addClientSocketsToFD_SET()
{
	//add child sockets to set  
	sem_wait(&sm_ClientList);
	for (int i = 0 ; i < MAX_CLIENTS ; i++)   
	{   
            //socket descriptor  
	    sd = client_sockets[i]; 
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET(sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }
	sem_post(&sm_ClientList);  
}
//pega clientlist
void checkIOServer()
{
	if (FD_ISSET(server_fd, &readfds)) 
	{ 
			if ((new_socket = accept(server_fd, 
					(struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
			{ 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 
			//add new socket to array of sockets 
			if (saveSocketList(new_socket) == -1)
			{
				dprintf(1,"NEW_SOCKET %ld REFUSED\n",new_socket);
			}			
	} 
}
//pega o lock de fila,  incrementa a quantidade de vezes que ele foi servido 
//caso a mensagem seja do tipo release 
//ou coloca na fila caso contrário
void addMessageQueue(int socketFd)
{
	parse(buffer);
	Message*msg = malloc(sizeof(Message));
	msg->type = (*clientMessageParsed[0] - '0');
	msg->id = atoi((clientMessageParsed[1]));
	msg->fd = socketFd;
	//dprintf(1,"Putting message (%d) from process %d\n",msg->type,msg->id);
	//checa se é do tipo release
	if(msg->type == 3)
	{
		releaseMessageReceived();
		incrementProcessCounter(msg->id);
	}
	else
	{
		//dprintf(1,"addMessageQueue Waiting Queue lock\n");
		sem_wait(&sm_Queue);
		//dprintf(1,"addMessageQueue SUCCESS\n");
		fila_insere(Q,msg);
		//dprintf(1,"FILA INSERE OK\n");
		sem_post(&sm_Queue);
	}
}
//Pega o lock de clients_list e fila
void checkIOclient()
{
	sem_wait(&sm_ClientList);  
	for (int i = 0; i < MAX_CLIENTS; i++)   
        {   
            
            sd = client_sockets[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = read( sd , buffer, 1024)) == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    //getpeername(sd , (struct sockaddr*)&address, (socklen_t*)&addrlen);   
                    //printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    //Close the socket and mark as 0 in list for reuse 
                    dprintf(1,"Thread %ld disconnected\n",client_sockets[i]);
                    if(clientWithGrant == sd)
                    {
                    	sem_post(&sm_noClientWithGrant);
                    }
                    close( sd );   
                    client_sockets[i] = 0;
                    
                }  
                else 
                {   
                    //set the string terminating NULL byte on the end  
                    //of the data read  
                    buffer[valread] = '\0';
                    addMessageQueue(sd); 
                }
       
            }
      	}
    	sem_post(&sm_ClientList);                      
}

void *mutualExclusion()
{
	int vazia = 0;
	Message*msg;
	while(KEEP_READING_QUEUE)
	{
		//dprintf(1,"Mutual exclusion waiting for noClientWithGrant\n");
		sem_wait(&sm_noClientWithGrant);
		//dprintf(1,"Mutual exclusion waiting for queue\n");
		sem_wait(&sm_Queue);
		vazia = fila_vazia(Q);
		sem_post(&sm_Queue);
		if(!vazia)
		{
			//dprintf(1,"Mutual exclusion waiting for queue2\n");
			sem_wait(&sm_Queue);
			msg = fila_retira(Q);
			//dprintf(1,"Mutual exclusion read message from head\n");
			sem_post(&sm_Queue);	
			//dprintf(1,"Mutual exclusion release queue lock\n");
			switch(msg->type)
			{
				case 1:
					//dprintf(1,"Mutual exclusion execute request\n");
					requestMessageReceived(msg);
					free(msg);
					//dprintf(1,"Mutual exclusion leave request\n");
				break;
							
				default:
					//dprintf(1,"Received an invalid message\n");
					sem_post(&sm_noClientWithGrant);
					close(msg->fd);
				break;
			}
		}
		else
		{
			sem_post(&sm_noClientWithGrant);	
		}
			
	}
}
//Pega client list e fila
void *handleConnection(void *arg)
{
	int pos = 0;
	//get the socket that triggered select function
	long unsigned int socketTriggeredSelect = 0;
	//check if the client socket disconnected or sent message
	int disconnected = 0;
	// Creating socket file descriptor IPV4 IPv6
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
	
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(server_fd, MAX_CLIENTS) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
	struct timeval tv;
    	tv.tv_sec = 1;
	while(KEEP_HANDLING) 
	{ 
		//dprintf(1,"%s","Inside HANDLER\n");
		//clear the socket set 
		FD_ZERO(&readfds); 
		FD_SET(server_fd, &readfds); 
		max_sd = server_fd; 
		addClientSocketsToFD_SET();
		//wait for an activity on one of the sockets , timeout is NULL , 
		//so wait indefinitely 
		activity = select( max_sd + 1 , &readfds , NULL , NULL , &tv); 
		if ((activity < 0) && (errno!=EINTR)) 
		{ 
			printf("select error"); 
		} 
		checkIOServer();
		checkIOclient();
	}
	sem_post(&sm_MAIN);
}

int main(int argc, char const *argv[]) 
{ 
	Q = fila_cria();
	int i=0;
	initializeProcessCounter();
	sem_init(&sm_MAIN,0,0);
	sem_init(&sm_Queue,0,1);
	sem_init(&sm_noClientWithGrant,0,1);
	sem_init(&sm_AtendimentoList,0,1);
	sem_init(&sm_ClientList,0,1);
	pthread_t serverThreads[3];
	pthread_t thread_id;
	pthread_create(&thread_id,NULL,mutualExclusion,NULL);
	serverThreads[2] = thread_id;
	pthread_create(&thread_id,NULL,handleConnection,NULL);
	serverThreads[1] = thread_id;
	printf("Server id %ld\n",thread_id);
	pthread_create(&thread_id,NULL,listenCLI,NULL);
	serverThreads[0] = thread_id;
	sem_wait(&sm_MAIN);
	return 0; 
} 

