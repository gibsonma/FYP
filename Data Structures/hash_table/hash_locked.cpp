/*lockless hash table
  If any single bucket exceeds global threshold, double table size - Could have a global atomic and could compare each bucket's length to it after an add

  (Pg 302/303) Coarse Grained - To resize table, lock the set, ensure table length is as expected, initialise and new table with double the capacity, transfer items from the old buckets to the new, unlock set
 */
#include <atomic>
#include <iostream>
#include "pthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "xmmintrin.h"
#define KEY_RANGE 128
//#define KEY_RANGE 131072
//#define KEY_RANGE 134217728
#define INIT_TABLE_SIZE 128
//#define INIT_TABLE_SIZE 131072
//#define INIT_TABLE_SIZE 134217728
#define MAX_THREAD_VAL 128
#define EXECUTION_TIME 1
#define PAUSE 2
#define MIN_DELAY 1
#define MAX_DELAY 10
#define TCOUNT//Tracks how many threads did which function
//#define SEARCH//Prints out the number of +tive/-tive searches
//#define DEBUG//Print out the table at the end of the program
//#define RESIZE//Defines the resizing functionality
#define MAX_LIST_LENGTH 10//If a list's length is greater than this, the table resizes

//Modes of Operation - If neither defined then defaults to assembly spinlock
#define LOCKED //Uses simple mutex blocking
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
volatile long long pSearches = 0;//Tracks positive contains() calls
volatile long long nSearches = 0;//Tracks negative contains() calls
int containsC = 0, addC = 0, removeC = 0;
struct Node{
	int key;
	Node * volatile next;
	Node(int key) : key(key), next(nullptr){}
};
struct List{
	Node * volatile head;
	Node * volatile tail;
	int listLength = 0;
	void add(int key);
};
struct Table{
	int numItems;
	int size;
	List ** table;
	Table(int tableLength) 
	{
		size = tableLength;
		table = new List*[size];
		for(int i = 0; i < size; i++)table[i] = NULL;
		numItems = 0;
	}
};
Table * htable = new Table(INIT_TABLE_SIZE);
int hashFunc(int item)
{
	return item % htable->size;
}

#if defined(RESIZE)
//Non threaded functions for moving buckets to a new table
//Only called when a lock is acquired so no need for additonal locking
void List::add(int key)
{
	Node * newNode = new Node(key);
	if(head == NULL || tail == NULL)
	{
		head = newNode;
		tail = newNode;
	}
	else
	{
		head->next = newNode;
		head = newNode;
	}
}
//Creates a new table of twice the size of the current table. Transfers all the buckets
//over to the new table before returning it
Table * resize(int oldTableLength)
{
	if(oldTableLength != htable->size)
	{
		return htable;
	}
	int newLength = oldTableLength * 2;
	Table * newTable = new Table(newLength);
	Node * tmpTail;
	Node * tmpHead;
	int hash;
	for(int i = 0; i < htable->size; i++)
	{
		if(htable->table[i] != NULL)
		{
			List * currList = htable->table[i];
			tmpTail = currList->tail;
			tmpHead = currList->head;
			while(tmpTail != NULL && tmpTail != tmpHead->next)
			{
				hash = tmpTail->key % newLength;
				if(newTable->table[hash] == NULL)newTable->table[hash] = new List();
				newTable->table[hash]->add(tmpTail->key);
				tmpTail = tmpTail->next;
			}
		}
	}
	return newTable;
}
#endif
void printTable()
{
	List * currBucket;
	Node * tmpTail; 
	Node * tmpHead;
	int count = 0;
	for(int i = 0; i < htable->size; i++)
	{
		if(htable->table[i] != NULL)
		{
			currBucket = htable->table[i];
			tmpTail = currBucket->tail;
			tmpHead = currBucket->head;
			cout << "List contained at index: " << i << " ";
			while(tmpTail != NULL && tmpTail != tmpHead->next)
			{
				//cout << tmpTail->key << " , ";
				count++;
				tmpTail = tmpTail->next;
			}

			cout << " Number of items in list: " << count << "\n";
			count = 0;
		}
	}
	cout << "\n";
}
#if defined(SEARCH)
void printSearchResults()
{
	cout << "Positive Searches: " << pSearches << " Negative Searches: " << nSearches << "\n";
}
#endif
#if defined(TCOUNT)
void printTCounts()
{
	cout << "Contains: " << containsC << " Adds: " << addC << " Removes: " << removeC << "\n";
}
#endif
void * add(void * threadid)
{
//	int count = 0;
//	while(count < 5)
	while(true)
	{
//count++;
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
		int hash = hashFunc(key);
//		cout << "Attempting to add " << key << " to index " << hash << "\n";
		Node * newNode = new Node(key);
		Node * tmpHead;
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		iterations++;
		if(tmpList == NULL)//If index in table is empty
		{
			htable->table[hash] = new List();
			tmpList = htable->table[hash];
			tmpList->head = newNode;
			tmpList->tail = newNode;
			tmpList->listLength = 1;
		}
		else if(tmpList->head == NULL || tmpList->tail == NULL)
		{
			tmpList->head = newNode;
			tmpList->tail = newNode;
			tmpList->listLength = 1;
		}
		else 
		{
			tmpList->head->next = newNode;
			tmpList->head = newNode;
			tmpList->listLength++;
		}
#if defined(RESIZE)
		if(tmpList->listLength >= MAX_LIST_LENGTH)
		{
			//		cout << "Table before resize\n";
			//		printTable();
			htable = resize(htable->size);
			//		cout << "Table after resize\n";
			//		printTable();
		}
#endif
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

void * remove(void * threadid)
{
	//int count = 0;
	//while(count < 5)
	while(true)
	{
//		count++;
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

		int hash = hashFunc(rand() % KEY_RANGE);
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		iterations++;
		if(tmpList != NULL)
		{
			tmpTail = tmpList->tail;
			if(tmpTail != NULL)
			{
				//cout << "Attempting to remove the tail " << tmpTail->key << "\n";
				tmpList->tail = tmpTail->next;
				tmpList->listLength--;
				if(tmpTail->next == NULL)
				{
					//cout << "List is now empty\n";
					tmpList->head = NULL;
					//cout << "Head set to null\n";
				}
				//else cout << "New tail is " << tmpTail->next->key << "\n";
			}
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
//Generates a random number and then searches the table for it
//Get hash;Get relevant list;search through list;
void * contains(void * threadid)
{
//	int count = 0;
//	while(count < 5)
	while(true)
	{
		pthread_mutex_lock(&listLock);
//		count++;
	//	srand(time(NULL));
		int key = rand() % KEY_RANGE;
		int hash = hashFunc(key);
//		cout << "Searching for value " << key << "\n";
		List * tmpList = htable->table[hash];
		Node * tmpTail;
		iterations++;
		if(tmpList)
		{
			tmpTail = tmpList->tail;
			while(tmpTail != NULL)
			{
				if(tmpTail->key == key)
				{
					pSearches++;
//					cout << "Positive Search on value " << key << "\n";
					break;
				}
				tmpTail = tmpTail->next;
			}
			if(tmpTail == NULL)
			{
//				cout << "Negative Serch on value " << key << "\n";
				nSearches++;
			}
		}
		else 
		{
//			cout << "Null List for value " << key << "\n";
			nSearches++;
		}
		pthread_mutex_unlock(&listLock);
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void * choose(void * threadid)
{
	int num = rand() % 10;
	if(num >= 5)
	{
		containsC++;
		contains(threadid);
	}
	else if(num >= 2)
	{
		addC++;
		add(threadid);
	}
	else 
	{
		removeC++;
		remove(threadid);
	}
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
			rc = pthread_create(&threads[t], NULL, choose, (void *)t);
		}
		for(t = 0; t < i; t++)
		{
			pthread_join(threads[t], NULL);
		}
		gettimeofday(&stop_time, NULL);
		total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
		printf("%lld ,\n",iterations/EXECUTION_TIME);
		//      printf("Total executing time %lld microseconds, %lld iterations/s  and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}
#if defined(DEBUG)
	printTable();
#endif
#if defined(SEARCH)
	printSearchResults();
#endif
#if defined(RESIZE)
	cout << "Final Size: " << htable->size << "\n";
#endif
#if defined(TCOUNT)
	printTCounts();
#endif
	return 0;
}
