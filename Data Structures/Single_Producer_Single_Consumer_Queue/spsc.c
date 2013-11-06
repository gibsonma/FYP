#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#define TIMING
#define EBUSY 1
//#define LOCKED
#define LOCKLESS

void acquire(int id);
void release(int id);
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
const long long MAX_ITERATIONS = 1000000;//Maximum number of iterations for the program

int flag[2];
int last;
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
#ifdef LOCKED		
		pthread_mutex_lock(&bufferLock);//Lock shared variable
		if(bufferIndex == BUFFER_SIZE)//If buffer is full then wait
		{                        
			pthread_cond_wait(&bufferNotFull,&bufferLock);
		}                        
		iterations++;
		BUFFER[bufferIndex++] = '@';
		pthread_mutex_unlock(&bufferLock);//Unlock shared variable
		pthread_cond_signal(&bufferNotEmpty);//Signal that buffer is not empty
#endif	
#ifdef LOCKLESS
		while(bufferIndex == BUFFER_SIZE);
		acquire(0);
		iterations++;
		BUFFER[bufferIndex++] = '@';
		release(0);
#endif
	}    
}

void *Consumer()
{
	while(iterations < MAX_ITERATIONS)
	{
#ifdef LOCKED		
		pthread_mutex_lock(&bufferLock);
		if(bufferIndex == 0)//If buffer is empty then wait
		{            
			pthread_cond_wait(&bufferNotEmpty,&bufferLock);
		}                
		iterations++;
		bufferIndex--;
		pthread_mutex_unlock(&bufferLock);        
		pthread_cond_signal(&bufferNotFull);//Signal that buffer is not full  
#endif
#ifdef LOCKLESS
		while(bufferIndex == BUFFER_MIN);
		acquire(1);
		iterations++;
		bufferIndex--;
		release(1);
#endif

	}    
}

void acquire(int id) // id is the thread ID [0 or 1] 
{ 
	int j = 1 - id; // 0 -> 1 and 1 ->0 
	flag[id] = 1; // want lock 
	last = id; // other thread has priority 
	while (flag[j] && last == id); 
} 

void release(int id) 
{ 
	flag[id] = 0; // release lock 
} 
