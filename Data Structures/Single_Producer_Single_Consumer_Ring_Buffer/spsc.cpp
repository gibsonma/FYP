#include <iostream>
#include <atomic>
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#define NUM_THREADS 2
#define SIZE 10000000
#define EXECUTION_TIME 1
#define PAUSE 2

using namespace std;

std::atomic<int> startB (0);//Read Position
std::atomic<int> endB(0);//Write Position
int * buffer;
int item, currentEnd, nextEnd, currentStart;
struct timeval start_time, stop_time;
long long total_time = 0, tstart = 0, tstop = 0;
volatile long long iterations = 0;

void * push(void* threadid)
{
	while(true)
	{
		item = (rand() % 100) + 1;
		iterations++;
		currentEnd = endB;
		nextEnd = (endB + 1) % SIZE;
		if(nextEnd != startB)
		{
			buffer[currentEnd] = item;
			endB = nextEnd;
		}
//		else sleep(PAUSE);
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) - (start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void * pop(void * threadid)
{
	while(true)
	{
		iterations++;
		currentStart = startB;
		if(currentStart == endB){}
		else
		{
			buffer[currentStart] = 0;
			startB = (startB + 1) % SIZE;
		}
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) - (start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void printBuffer()
{
	cout << "\nBuffer: \n";
	for(int i = 0; i < SIZE; i++)
	{
		cout << buffer[i] << endl;
	}
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
	for(t = 0; t < NUM_THREADS; t++)pthread_join(threads[t], NULL);
	gettimeofday(&stop_time, NULL);
	total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
	printf("%lld ,",iterations/EXECUTION_TIME);
	iterations = 0;
	return 0;
}
