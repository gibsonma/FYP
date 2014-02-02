// Implementation of a multithreaded ring buffer with
// several different modes of operation
// Author: Mark Gibson

#include <iostream>
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <atomic>
#include <unistd.h>
#define NUM_THREADS 1
#define MAX_THREAD_VAL 128
#define SIZE 1000000
#define EXECUTION_TIME 1
#define PAUSE 2
#define MIN_DELAY 1
#define MAX_DELAY 10

//Modes of Operation - If neither defined then defaults to assembly spinlock
//#define LOCKED //Uses simple mutex blocking
//#define SPINLOCK //Uses a spinlock implemented with C++ atomics
//#define CASLOCK
#define TAS
//#define TICKET

pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;

using namespace std;

struct timeval start_time, stop_time;
long long total_time = 0, tsart = 0, tstop = 0;
volatile long long iterations = 0;
int size = SIZE; //Max number of elements
int back, front, count = 0; //Index of oldest element / Index to write new element / Number of elements in buffer
int *buffer; //Buffer of elements
volatile int CASlock = 0;
std::atomic<int> lock (0);
int notTaken  = 0, taken = 1;//For the CAS lock
volatile long nowServing = 0;//For ticket lock
std::atomic<long> ticket (0);//For ticket lock
void* push(void* threadid)
{
	while(1)
	{
#if defined(LOCKED)
		pthread_mutex_lock(&bufferLock);	//Blocking Implementation
#elif defined(SPINLOCK) 
		do{					//C++ spinlock
			while(lock.load() == 1)usleep(PAUSE);
		}while(lock.exchange(1));
#elif defined(CASLOCK)
		int delay = MIN_DELAY;
		while(true)
		{
			if(lock.compare_exchange_strong(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(TAS)
		while(lock.exchange(1));
#elif defined(TICKET)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)sleep(myTicket - nowServing);
#endif
		int item = rand() % 100;
		iterations++;
		if (front == 0 && back == size || front == back + 1) {
		}
		else if (front == -1 && back == -1) {
			front = 0;
			back = 0;
			buffer[front] = item;
			count++;
		}
		else if (back == size) {
			back = 0;
			buffer[back] = item;
			count++;
		}
		else {
			back++;
			buffer[back] = item;
			count++;
		}
#if defined(LOCKED)		
		pthread_mutex_unlock(&bufferLock);
#elif defined(SPINLOCK)
		lock = 0;
#elif defined(CASLOCK)
		lock = 0;
#elif defined(TAS)
		lock = 0;
#elif defined(TICKET)
		nowServing++;
#endif	
		gettimeofday(&stop_time, NULL);//If the thread has run for one second or more then stop
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void* pop(void* threadid)
{
	while(1)
	{
#if defined(LOCKED)
		pthread_mutex_lock(&bufferLock);
#elif defined(SPINLOCK)
		do{
			while(lock.load() == 1)usleep(PAUSE);
		}while(lock.exchange(1));
#elif defined(CASLOCK)
		int delay = MIN_DELAY;
		while(true)
		{
			if(lock.compare_exchange_strong(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(TAS)
		while(lock.exchange(1));
#elif defined(TICKET)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)sleep(myTicket - nowServing);
#endif
		iterations++;
		if (front == -1 && back == -1) {
		}
		else {
			if (front == back) {
				buffer[front] = 0;
				front = -1;
				back = -1;
				count--;
			}
			else if (front == size) {
				buffer[front] = 0;
				front = 0;
				count--;
			}
			else {
				buffer[front] = 0;
				front++;
				count--;
			}
		}
#if defined(LOCKED)
		pthread_mutex_unlock(&bufferLock);
#elif defined(SPINLOCK)
		lock = 0;
#elif defined(CASLOCK)
		lock = 0;
#elif defined(TAS)
		lock = 0;
#elif defined(TICKET)
		nowServing++;
#endif
		gettimeofday(&stop_time, NULL);//If the thread has run for one second or more then stop
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void printBuffer()
{
	for(int i = 0; i < size; i++)
	{
		cout << buffer[i] << endl;
	}
	cout << "\n" << endl;
}

int main()
{
	for(int i = 1; i <= MAX_THREAD_VAL; i = i * 2){
		srand(time(NULL));
		gettimeofday(&start_time, NULL);
		int rc, t;
		buffer = new int[SIZE];
		pthread_t threads[i];
		for(t = 0; t < i; t++)
		{
			if(t % 2 == 0)rc = pthread_create(&threads[t], NULL, push, (void *)t);
			else rc = pthread_create(&threads[t], NULL, pop, (void *)t);
		}
		for(t = 0; t < i; t++)
		{
			pthread_join(threads[t], NULL);
		}
		gettimeofday(&stop_time, NULL);
		total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
		printf("%lld ,",iterations/EXECUTION_TIME);
//		printf("Total executing time %lld microseconds with %lld iterations per second and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}

	return 0;
}
