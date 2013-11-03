#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#define TIMING
#define LOCKED
//#define LOCKLESSS


void *Producer();
void *Consumer();

#ifdef TIMING
	struct timeval start_time;
	struct timeval stop_time;
	long long total_time = 0;
#endif

const int BUFFER_SIZE = 10;//Maximum size of buffer
const int BUFFER_MIN = 0;//Minimim size of buffer
const int NUM_THREADS = 2;//Number of threads used
const long long MAX_ITERATIONS = 1000;//Maximum number of iterations for the program

int bufferIndex = 0;//Tracks number of items in buffer
long long iterations = 0;//Tracks number of iterations completed
char *BUFFER;//Buffer represented by array of chars

pthread_cond_t bufferNotFull=PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotEmpty=PTHREAD_COND_INITIALIZER;
pthread_mutex_t bufferLock=PTHREAD_MUTEX_INITIALIZER;

int main(int argc, const char * argv[])
{    
#ifdef TIMING
	gettimeofday(&start_time, NULL);//Get starting time
#endif
	int t, rc;
	pthread_t threads[NUM_THREADS];
	BUFFER = (char *) malloc(sizeof(char) * BUFFER_SIZE);            
	srand(time(NULL));
	for(t = 0; t < NUM_THREADS; t++)
	{
		printf("Creating thread %d\n", t);
		if(t % 2 == 0)rc = pthread_create(&threads[t], NULL, Producer, (void *) t);
		else rc = pthread_create(&threads[t], NULL, Consumer, (void *) t);

	}

	for(t = 0; t < NUM_THREADS; t++)
	{
		pthread_join(threads[t], NULL);
	}
#ifdef TIMING
	gettimeofday(&stop_time, NULL);
	total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
	printf("Total executing time %lld microseconds with %lld iterations\n", total_time, MAX_ITERATIONS);
#endif
	return 0;
}

void *Producer(void * threadid)
{    
	while(iterations < MAX_ITERATIONS)
	{
		pthread_mutex_lock(&bufferLock);//Lock shared variable
		if(bufferIndex == BUFFER_SIZE)//If buffer is full then wait
		{                        
			pthread_cond_wait(&bufferNotFull,&bufferLock);
		}                        
		printf("Produce : %d \n", bufferIndex);
		iterations++;
		BUFFER[bufferIndex++] = '@';
		pthread_mutex_unlock(&bufferLock);//Unlock shared variable
		pthread_cond_signal(&bufferNotEmpty);//Signal that buffer is not empty
	}    
}

void *Consumer()
{
	while(iterations < MAX_ITERATIONS)
	{
		pthread_mutex_lock(&bufferLock);
		if(bufferIndex == 0)//If buffer is empty then wait
		{            
			pthread_cond_wait(&bufferNotEmpty,&bufferLock);
		}                
		printf("Consume : %d \n", bufferIndex);
		iterations++;
		bufferIndex--;
		pthread_mutex_unlock(&bufferLock);        
		pthread_cond_signal(&bufferNotFull);//Signal that buffer is not full  
	}    
}
