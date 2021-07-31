#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h> 

int *sharedMemory;//Size N (memLen)
int *consumerMemory;
int memLen;
sem_t sm_mtx;//Mutex for the shared mem
sem_t sm_MAIN;//Mutex that prevents Main executing before consumers have finished
sem_t empty;
//Count the number of shared mem positions are empty (At first N)
sem_t full;
//Count the number of shared mem positions are full (At first none)
int conMemPos = 0;//keep track of the next free position of consumerMemory
int produced = 0;//Count the amount of numbers already produced
int consumed = 0;//Count the amount of numbers already consumed
int totalToProduce =100000;//total numbers to be generated
int KEEP_CONSUMING = 1;//condition variable to exit loop
int KEEP_PRODUCING = 1;//condition variable to exit loop
int sval1;
int sval2;
int isPrime(long n) 
{
	int i; 
	for (i = 2; i <= n / 2; ++i) 
	{
		if (n % i == 0)
            		return 0;
        }
	return 1;
}

int getRandomNumber(){
    int number = rand() % 10000001;
    return number;
}

int getEmptyPosition(int *sharedMem){
	for(int i = 0; i < memLen; i++)
	{
		if(sharedMem[i] == 0)
			return i;

	}
	return -1;
}

int getFilledPosition(int *sharedMem){
	for(int i = 0; i < memLen; i++)
	{
		if(sharedMem[i] != 0)
			return i;

	}
	return -1;
}

void produce(){
	int rand = getRandomNumber();	
	int pos = getEmptyPosition(sharedMemory);
	if(pos != -1)
		sharedMemory[pos] = rand;
}

void *producer(void *arg)
{
	while(KEEP_PRODUCING)
	{
		sem_wait(&empty);
		//Only if shared memory has free space the producer will produce, it means, if semaphore has its counter > 0
		sem_wait(&sm_mtx);
		//only one thread can access the shared memory at time
		if(produced < totalToProduce)
		{
			produce();
			produced++;
		}
		else
		{
			KEEP_PRODUCING = 0;
		}
		sem_post(&sm_mtx);
		//allow other threads access since it has already produced
		sem_post(&full);
		//It sends a signal allowing that any waiting consumer be waked up
	
	}

}

void consume(){
	int pos = getFilledPosition(sharedMemory);
	if(pos != -1)
	{
		consumerMemory[conMemPos] = sharedMemory[pos];
		printf("%d %s",sharedMemory[pos],isPrime(sharedMemory[pos])?"is prime\n":"is not prime\n");
		sharedMemory[pos] = 0;
		conMemPos++;
	}

}

void *consumer(void *arg)
{
	while(KEEP_CONSUMING)
	{
		sem_wait(&full);
		//Only if shared memory has content the consumer will execute, it means, if semaphore has its counter > 0
		sem_wait(&sm_mtx);
		//only one thread can access the shared memory at time
		if(consumed < totalToProduce)
		{
			consume();
			consumed++;
		}
		else
		{
			
			KEEP_CONSUMING = 0;
		}
		sem_post(&sm_mtx);
		//allow other threads access since it has already consumer
		sem_post(&empty);
		//It sends a signal allowing that any waiting producer be waked up as now there is free space
		
	}

}

int main(int argc, char* argv[]) {
	time_t seed;
	srand((unsigned) time(&seed));
	void *status;//variable required in pthread_join
	memLen = atoi(argv[1]); // first argument is N, the number os spaces in shared memory
	int consumerThreadsNo = atoi(argv[2]);  // second argument is the number of consumer threads
	int producerThreadsNo = atoi(argv[3]);  // third argument is the number of producer threads
	pthread_t thread_id;
	sem_init(&empty,0,memLen);
	sem_init(&full,0,0);
	sem_init(&sm_mtx,0,1);
	sem_init(&sm_MAIN,0,0);

	printf("Consumer:%d Producer:%d\n",consumerThreadsNo,producerThreadsNo);
	// Measure of time 
	clock_t t; 
	t = clock();
	int numberOfThreads = consumerThreadsNo+producerThreadsNo;
	pthread_t producerConsumerThreads[numberOfThreads];//create an array to keep track of all created threads
	sharedMemory =  (int*) malloc(memLen*sizeof(int));
	consumerMemory =  (int*) malloc(totalToProduce*sizeof(int));
	// Fill memory vector with the argument spaces
	for (int i = 0; i < memLen; ++i){
		sharedMemory[i] = 0;
	}
	// Launch  Threads
	for(int j = 0; j < consumerThreadsNo;j++){
		pthread_create(&thread_id,NULL,consumer,NULL);
		producerConsumerThreads[j] = thread_id;
	}
	for(int k =0 ; k < producerThreadsNo;k++){
		pthread_create(&thread_id,NULL,producer,NULL);
		producerConsumerThreads[k] = thread_id;
	}
	//Impede da Main executar antes que todos os consumidores tenham acabado. O consumidor envia signal(sm_Main) quando a quantidade consumida atinge o esperado
	sem_wait(&sm_MAIN);
	t = clock() - t;
        double elapsed_seconds = ((double) t)/CLOCKS_PER_SEC;
    	elapsed_seconds = elapsed_seconds;
	printf("Consumed:%d\nProduced:%d",consumed,produced);
	char buffer[20];
	sprintf(buffer,"%i_%i_%i\0",memLen,consumerThreadsNo,producerThreadsNo);
	FILE *fptr;
	fptr = fopen(buffer,"a");
	fprintf(fptr,"%f\n",elapsed_seconds);
	fclose(fptr);
	return 0;
}
