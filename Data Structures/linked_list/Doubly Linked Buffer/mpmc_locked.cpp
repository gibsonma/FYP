// Implementation of a multithreaded linked list with
// several different modes of operation, based on code written by Dr. Jermey Jones of TCD
// Author: Mark Gibson

#include <iostream>
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <atomic>
#include <unistd.h>
#include "xmmintrin.h"
#include "median.h"
#define NUM_THREADS 1
#define MAX_THREAD_VAL 128
#define KEY_RANGE 128
#define EXECUTION_TIME 1
#define PAUSE 2
#define MIN_DELAY 1
#define MAX_DELAY 10
//#define DEBUG
//Modes of Operation - If neither defined then defaults to assembly spinlock
//#define LOCKED //Uses simple mutex blocking
//#define TTAS //Uses a spinlock implemented with C++ atomics
//#define TTASNP
//#define CASLOCK
//#define CASLOCKND
//#define TAS
//#define TASWP
//#define TICKET
//#define TTAS_RELAX
//#define CASLOCK_RELAX
//#define TAS_RELAX
//#define TICKET_RELAX

pthread_mutex_t listLock = PTHREAD_MUTEX_INITIALIZER;

using namespace std;

struct timeval start_time, stop_time;
long long total_time = 0, tsart = 0, tstop = 0;
volatile long long iterations = 0;
volatile int CASlock = 0;
std::atomic<int> lock (0);
int notTaken  = 0, taken = 1;//For the CAS lock
volatile long nowServing = 0;//For ticket lock
std::atomic<long> ticket (0);//For ticket lock
long long results[8];
void printList();
struct Node{

	volatile int key;
	Node * volatile next;
	Node * volatile prev;
	Node(int key) : key(key), next(nullptr), prev(nullptr){}
};
Node * volatile head = NULL;
Node * volatile tail = NULL;

void* add(void* threadid)
{
	while(true)
	{
#if defined(LOCKED)
		pthread_mutex_lock(&listLock);
#elif defined(TTAS)
		do{
			while(lock.load() == 1)sleep(PAUSE);
		}while(lock.exchange(1));
#elif defined(TTAS_RELAX)
		do{
			while(lock.load() == 1)_mm_pause();
		}while(lock.exchange(1));
#elif defined(TTASNP)
		do{
			while(lock.load() == 1);
		}while(lock.exchange(1));
#elif defined(CASLOCK)
		int delay = MIN_DELAY;
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(CASLOCK_RELAX)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
			_mm_pause();
		}
#elif defined(CASLOCKND)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
		}
#elif defined(TAS)
		while(lock.exchange(1));
#elif defined(TASWP)
		while(lock.exchange(1))sleep(PAUSE);
#elif defined(TAS_RELAX)
		while(lock.exchange(1))_mm_pause();
#elif defined(TICKET)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)sleep(myTicket - nowServing);
#elif defined(TICKET_RELAX)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)_mm_pause();
#endif
		int key = rand() % KEY_RANGE;
		iterations++;
		//cout << "Adding " << key << "\n";
		Node * newNode = new Node(key);
		Node * volatile tmpHead = head;
		Node * volatile tmpTail = tail;
		if(tmpHead == NULL)//If linked list is empty
		{
			head = newNode;//Set head to be new node
			tail = newNode;//Set tail to point to start of list
		}
		else
		{
			newNode->next = tmpHead;
			tmpHead->prev = newNode;
			head = newNode;
		}
#if defined(LOCKED)
		pthread_mutex_unlock(&listLock);
#elif defined(TICKET)
		nowServing++;
#elif defined(TICKET_RELAX)
		nowServing++;
#else
		lock = 0;
#endif
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}

}

void* remove(void* threadid)
{
	while(true)
	{
#if defined(LOCKED)
		pthread_mutex_lock(&listLock);
#elif defined(TTAS)
		do{
			while(lock.load() == 1)sleep(PAUSE);
		}while(lock.exchange(1));
#elif defined(TTAS_RELAX)
		do{
			while(lock.load() == 1)_mm_pause();
		}while(lock.exchange(1));
#elif defined(TTASNP)
		do{
			while(lock.load() == 1);
		}while(lock.exchange(1));
#elif defined(CASLOCK)
		int delay = MIN_DELAY;
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(CASLOCK_RELAX)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
			_mm_pause();
		}
#elif defined(CASLOCKND)
		while(true){
			if(lock.compare_exchange_weak(notTaken, taken))break;
		}
#elif defined(TAS)
		while(lock.exchange(1));
#elif defined(TASWP)
		while(lock.exchange(1))sleep(PAUSE);
#elif defined(TAS_RELAX)
		while(lock.exchange(1))_mm_pause();
#elif defined(TICKET)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)sleep(myTicket - nowServing);
#elif defined(TICKET_RELAX)
		int myTicket = ticket.fetch_add(1);
		while(myTicket != nowServing)_mm_pause();
#endif
		iterations++;
		Node * volatile tmpHead = head;
		Node * volatile tmpTail = tail;
		Node * volatile tmpNode;
		if(tmpTail != NULL)//If linked list is empty then return
		{
			tmpNode = tmpTail->prev;
			//cout << "Removing " << tmpTail->key << "\n";
			tmpTail->key = 0;
			tail = tmpNode;

		}
#if defined(LOCKED)
		pthread_mutex_unlock(&listLock);
#elif defined(TICKET)
		nowServing++;
#elif defined(TICKET_RELAX)
		nowServing++;
#else
		lock = 0;
#endif
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}


void printList()
{
	Node * tmpNode = head;
	cout << "List:";
	while(tmpNode != NULL)
	{
		cout << tmpNode->key << ",";
		tmpNode = tmpNode->next;
	}
	cout << "\n";
}

int main()
{
	for(int i = 1; i <= MAX_THREAD_VAL; i = i * 2){
		for(int j = 0; j < 7; j++)
		{
			srand(time(NULL));
			gettimeofday(&start_time, NULL);
			int rc, t;
			pthread_t threads[i];
			for(t = 0; t < i; t++)
			{
				if(t % 2 == 0)rc = pthread_create(&threads[t], NULL, add, (void *)t);
				else rc = pthread_create(&threads[t], NULL, remove, (void *)t);
			}
			for(t = 0; t < i; t++)
			{
				pthread_join(threads[t], NULL);
			}
			gettimeofday(&stop_time, NULL);
			total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
			//printf("%lld ,",iterations/EXECUTION_TIME);
			results[j] = iterations/EXECUTION_TIME;
			iterations = 0;
		}
		getMedian(results, 8);
	}
	cout << "\n";
#if defined(DEBUG)
	cout << "Final list: "; printList();
#endif
	return 0;
}

