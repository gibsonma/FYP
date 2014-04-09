/*
  A single producer, single consumer buffer that uses the C++ 11 atomic library to implement a 
  lockless algorithm
*/
#include <iostream>
#include <atomic>
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#define NUM_THREADS 2
#define SIZE 128
#define KEY_RANGE 128
#define EXECUTION_TIME 1
#define PAUSE 2

using namespace std;

std::atomic<int> startB (0);//Read Position
std::atomic<int> endB(0);//Write Position
int * buffer;
int item, currentEnd, nextEnd, currentStart;
struct timeval start_time, stop_time;
long long total_time = 0, tstart = 0, tstop = 0;
std::atomic<int> iterations = ATOMIC_VAR_INIT(0);
long long results[8];
void * push(void* threadid)
{
	while(true)
	{
		item = rand() % KEY_RANGE;
		std::atomic_fetch_add(&iterations, 1);
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
		std::atomic_fetch_add(&iterations, 1);
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


int main()
{
	for(int i = 0; i < 7; i++)
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
		//printf("%lld ,\n",iterations/EXECUTION_TIME);
		results[i] = iterations/EXECUTION_TIME;
		iterations = 0;
	}
	getMedian(results, 8);
	return 0;
}
