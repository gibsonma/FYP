// Implementation of a ring buffer
// Author: Mark Gibson

#include <iostream>
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cstdatomic>
#define NUM_THREADS 32
#define SIZE 100
#define EXECUTION_TIME 1
#define PAUSE 2

//#define LOCKED
pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;

using namespace std;
struct timeval start_time;
struct timeval stop_time;
long long total_time = 0;
long long tstart = 0,  tstop = 0;
volatile long long iterations = 0;
int size = SIZE; //Max number of elements
int back, front, count = 0; //Index of oldest element / Index to write new element / Number of elements in buffer
int *buffer; //Buffer of elements
std::atomic<int> lock (0);

//#define EBUSY 1
#define cpu_relax() asm volatile("pause\n": : :"memory")
#define barrier() asm volatile("": : :"memory")
static inline unsigned xchg_32(void *ptr, unsigned x)
{
	__asm__ __volatile__("xchgl %0,%1"
			:"=r" ((unsigned) x)
			:"m" (*(volatile unsigned *)ptr), "0" (x)
			:"memory");

	return x;
}
typedef unsigned spinlock;
spinlock spinLock;
static void spin_lock(spinlock *lock)
{
	while (1)
	{
		if (!xchg_32(lock, EBUSY)) return;

		while (*lock) cpu_relax();
	}
}

static void spin_unlock(spinlock *lock)
{
	barrier();
	*lock = 0;
}

static int spin_trylock(spinlock *lock)
{
	return xchg_32(lock, EBUSY);
}
void* push(void* threadid)
{
	while(1)
	{
#ifdef LOCKED
		pthread_mutex_lock(&bufferLock);
#else
		do{
			while(lock.load() == 1)usleep(PAUSE);
		}while(lock.exchange(1));
		//		while(spin_trylock(&spinLock) == 0);
		//		spin_lock(&spinLock);
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
#ifdef LOCKED		
		pthread_mutex_unlock(&bufferLock);
#else
		lock = 0;
		//spin_unlock(&spinLock);
#endif	
		gettimeofday(&stop_time, NULL);//If the thread has run for one second or more then stop
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void* pop(void* threadid)
{
	while(1)
	{
#ifdef LOCKED
		pthread_mutex_lock(&bufferLock);
#else
		do{
			while(lock.load() == 1)usleep(PAUSE);
		}while(lock.exchange(1));
		//		while(spin_trylock(&spinLock) == 0);
		//		spin_lock(&spinLock);
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
#ifdef LOCKED
		pthread_mutex_unlock(&bufferLock);
#else
		lock = 0;
		//		spin_unlock(&spinLock);
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
	srand(time(NULL));
	gettimeofday(&start_time, NULL);
	int rc, t;
	buffer = new int[SIZE];
	pthread_t threads[NUM_THREADS];
	for(t = 0; t < NUM_THREADS; t++)
	{
		if(t % 2 == 0)rc = pthread_create(&threads[t], NULL, push, (void *)t);
		else rc = pthread_create(&threads[t], NULL, pop, (void *)t);
	}
	for(t = 0; t < NUM_THREADS; t++)
	{
		pthread_join(threads[t], NULL);
	}
	gettimeofday(&stop_time, NULL);
	total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
	printf("Total executing time %lld microseconds with %lld iterations per second and %d threads\n", total_time, iterations/EXECUTION_TIME, NUM_THREADS);

	return 0;
}
