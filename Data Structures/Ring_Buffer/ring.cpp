/*
  Implementation of a multi producer, multi consumer ring buffer using the C++ atomic library to implement
  lockless functionality
*/
#include <iostream>
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <atomic>
#include <unistd.h>
#include "xmmintrin.h"
#define NUM_THREADS 1
#define MAX_THREAD_VAL 128
#define KEY_RANGE 128
#define SIZE 131072
#define EXECUTION_TIME 1
#define PAUSE 2
#define MIN_DELAY 1
#define MAX_DELAY 10

//Modes of Operation - If neither defined then defaults to assembly spinlock
//#define LOCKED //Uses simple mutex blocking
//#define TTAS //Uses a spinlock implemented with C++ atomics
//#define TTASNP //TTAS with no pause instruction
//#define CASLOCK
//#define TAS
//#define TASWP // TAS with pause instruction 
//#define TICKET
//#define CASLOCKND
//#define TTAS_RELAX
//#define CASLOCK_RELAX
//#define TICKET_RELAX
//#define TAS_RELAX

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
long long results[8];
void* push(void* threadid)
{
	while(1)
	{
#if defined(LOCKED)
		pthread_mutex_lock(&bufferLock);	//Blocking Implementation
#elif defined(TTAS) 
		do{					//C++ spinlock
			while(lock.load() == 1)sleep(PAUSE);
		}while(lock.exchange(1));
#elif defined(TTASNP)
		do
		{
			while(lock.load() == 1);
		}while(lock.exchange(1));
#elif defined(CASLOCK)
		int delay = MIN_DELAY;
		while(true)
		{
			if(lock.compare_exchange_weak(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(TAS)
		while(lock.exchange(1));
#elif defined(TASWP)
		while(lock.exchange(1))sleep(PAUSE);
#elif defined(TICKET)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)sleep(myTicket - nowServing);
#elif defined(TTAS_RELAX)
		do{
			while(lock.load() == 1)_mm_pause();
		}while(lock.exchange(1));
#elif defined(CASLOCKND)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
		}
#elif defined(CASLOCK_RELAX)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
			_mm_pause();
		}
#elif defined(TAS_RELAX)
		while(lock.exchange(1))_mm_pause();
#elif defined(TICKET_RELAX)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)_mm_pause();
#endif
		int item = rand() % KEY_RANGE;
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
#elif defined(TICKET)
		nowServing++;
#elif defined(TICKET_RELAX)
		nowServing++;
#else
		lock = 0;
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
#elif defined(TTAS)
		do{
			while(lock.load() == 1)usleep(PAUSE);
		}while(lock.exchange(1));
#elif defined(TTASNP)
		do
		{
			while(lock.load() == 1);
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
#elif defined(TASWP)
		while(lock.exchange(1))sleep(PAUSE);
#elif defined(TICKET)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)sleep(myTicket - nowServing);
#elif defined(TTAS_RELAX)
		do{
			while(lock.load() == 1)_mm_pause();
		}while(lock.exchange(1));
#elif defined(CASLOCKND)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
		}
#elif defined(CASLOCK_RELAX)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
			_mm_pause();
		}
#elif defined(TAS_RELAX)
		while(lock.exchange(1))_mm_pause();
#elif defined(TICKET_RELAX)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)_mm_pause();
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
#elif defined(TICKET)
		nowServing++;
#elif defined(TICKET_RELAX)
		nowServing++;
#else
		lock = 0;
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


void getMedian(long long results[], int size)
{
	long long* sortedResults = new long long[size];
	for (int i = 0; i < size; ++i) {
		sortedResults[i] = results[i];
	}
	for (int i = size - 1; i > 0; --i) {
		for (int j = 0; j < i; ++j) {
			if (sortedResults[j] > sortedResults[j+1]) {
				long long tmp = sortedResults[j];
				sortedResults[j] = sortedResults[j+1];
				sortedResults[j+1] = tmp;
			}
		}
	}
	long median = 0.0;
	if ((size % 2) == 0) {
		median = (sortedResults[size/2] + sortedResults[(size/2) - 1])/2.0;
	} else {
		median = sortedResults[size/2];
	}
	cout << median << ",";
}
main()
{
	for(int i = 1; i <= MAX_THREAD_VAL; i = i * 2){
		for(int j = 0; j < 7; j++)
		{
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
		//printf("%lld ,",iterations/EXECUTION_TIME);
		//		printf("Total executing time %lld microseconds with %lld iterations per second and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		results[j] = iterations/EXECUTION_TIME;
		iterations = 0;
		}
		getMedian(results, 8);
	}

	return 0;
}
