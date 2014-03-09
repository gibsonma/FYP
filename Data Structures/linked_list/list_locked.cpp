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
#define CASLOCKND
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
void printList();
class Node{
	public:
		volatile int key;
		Node * volatile next;
		Node(int, Node *);
};
Node::Node(int val, Node * ptr)
{
	key = val;
	next = ptr;
}
Node * volatile head = NULL;

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
			if(lock.compare_exchange_strong(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(CASLOCK_RELAX)
		while(true){
			if(lock.compare_exchange_strong(notTaken, taken))break;
			_mm_pause();
		}
#elif defined(CASLOCKND)
		while(true){
			if(lock.compare_exchange_strong(notTaken, taken))break;
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
		//cout << "Attempting to add " << key << "\n";
		Node * volatile tmpHead = head;
		Node * newNode = new Node(key, NULL);
		Node * volatile tmpNode = head;
		Node * volatile prevNode;
		if(tmpNode == NULL)//If linked list is empty
		{
			//	cout << "Linked list is empty \n";
			head = newNode;//Set head to be new node

		}
		else if(tmpNode->key > newNode->key)//If new key is smaller than head
		{
			//	cout << "New node is smaller than head \n";
			newNode->next = tmpNode;
			head = newNode;

		}
		else//Traverse list until entry point found
		{
			while(tmpNode && (tmpNode->key < newNode->key))
			{
				prevNode = tmpNode;
				tmpNode = tmpNode->next;
			}
			if(tmpNode && (tmpNode->key == newNode->key))
			{
				//		cout << "Node already present\n";
			}
			else
			{
				//		cout << "Adding Node\n";
				newNode->next = tmpNode;
				prevNode->next = newNode;
			}

		}
		//	printList();
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
			if(lock.compare_exchange_strong(notTaken, taken))break;
			sleep(rand() % delay);
			if(delay < MAX_DELAY)delay = 2 * delay;
		}
#elif defined(CASLOCK_RELAX)
		while(true){
			if(lock.compare_exchange_strong(notTaken, taken))break;
			_mm_pause();
		}
#elif defined(CASLOCKND)
		while(true){
			if(lock.compare_exchange_strong(notTaken, taken))break;
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
		//cout << "Attempting to remove " << key << "\n";
		Node * tmpNode = head;
		Node * prevNode;
		if(tmpNode == NULL)//If linked list is empty then return
		{
			//	cout << "List is empty\n";

		}
		else if(tmpNode->key == key)//Else if node is head then reassign head
		{
			head = tmpNode->next;
			delete tmpNode;
		}
		else//Else traverse list
		{
			while(tmpNode && (tmpNode->key != key))
			{
				prevNode = tmpNode;
				tmpNode = tmpNode->next;
			}
			if(tmpNode && (tmpNode->key == key))
			{
				//		cout << "Removing node" << key << "\n";
				prevNode->next = tmpNode->next;
			}
			//	else cout <<"Node does not exist\n";
		}
		//	printList();
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
		printf("%lld ,\n",iterations/EXECUTION_TIME);
	//	printf("Total executing time %lld microseconds with %lld iterations per second and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}
#if defined(DEBUG)
	cout << "Final list: "; printList();
#endif
	return 0;
}
