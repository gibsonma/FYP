/*
  Locked hash table, instead of locking down the table while entering a critical section, each list contains
  its own lock, meaning that multiple threads can act on the table at once
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
#define SEARCH//Prints out the number of +tive/-tive searches
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
	pthread_mutex_t nodeLock = PTHREAD_MUTEX_INITIALIZER;
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
int hashFunc(int item, Table * table)
{
	return item % table->size;
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
	Table * newTable = new Table(newLength);//Allocate new table
	Node * tmpTail;
	Node * tmpHead;
	Node * tmpNode;
	Node * prevNode;
	Node * specNode;
	List * newList;
	List * oldList;
	int hash, altHash;
	int prevHash;
	int key;
	for(int i = 0; i < newTable->size; i++)//For each new bucket
	{
		newTable->table[i] = new List();
		newList = newTable->table[i];
		cout << "New bucket: " << i << " Old Bucket: " << i%oldTableLength << "\n";
		oldList = htable->table[i%oldTableLength];//Get corresponding old bucket
		tmpTail = oldList->tail;
		while(tmpTail != NULL)//Search old bucket for first item that hashes to new table
		{
			key = tmpTail->key;
			hash = hashFunc(key,newTable);
			altHash = hashFunc(key, htable);
			cout << "Node key: " << key << " Hash of newTable: " << hash << " Hash of old table: " << altHash << "\n";
			if(hash == altHash)//If entry maps to new hash
			{
				newList->tail = tmpTail;//Link new bucket to entry
				cout << "New bucket linked to node: " << tmpTail->key << "\n";
				break;
			}
			tmpTail = tmpTail->next;//Else check next entry in bucket
		}
		/*
		if(newList->tail != NULL)
		{
			tmpTail = newList->tail;
			prevNode = tmpTail;
			prevHash = hashFunc(prevNode->key, newTable);
			tmpTail = tmpTail->next;
			hash = hashFunc(tmpTail->key, newTable);
			if(prevHash != hash)
			{
				specNode = prevNode;
				//TODO Break this up into functions and test it
			}
		}
		*/
	}
	return newTable;
}
#endif
void printTable()
{
	List * currBucket;
	Node * tmpTail; 
	Node * tmpHead;
	int count = 0, highC = 0;
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
				if(tmpTail->key > highC)highC = tmpTail->key;
				tmpTail = tmpTail->next;
			}

			cout << " Number of items in list: " << count << " Highest key: " << highC << "\n";
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
		int key = rand() % KEY_RANGE;
		int hash = hashFunc(key, htable);
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
			pthread_mutex_lock(&tmpList->nodeLock);
			tmpList->head = newNode;
			tmpList->tail = newNode;
			tmpList->listLength = 1;
			pthread_mutex_unlock(&tmpList->nodeLock);
		}
		else if(tmpList->head == NULL || tmpList->tail == NULL)
		{
			pthread_mutex_lock(&tmpList->nodeLock);
			tmpList->head = newNode;
			tmpList->tail = newNode;
			tmpList->listLength = 1;
			pthread_mutex_unlock(&tmpList->nodeLock);
		}
		else 
		{
			pthread_mutex_lock(&tmpList->nodeLock);
			tmpList->head->next = newNode;
			tmpList->head = newNode;
			tmpList->listLength++;
			pthread_mutex_unlock(&tmpList->nodeLock);
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
		int hash = hashFunc(rand(), htable);
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		iterations++;
		if(tmpList != NULL)
		{
			pthread_mutex_lock(&tmpList->nodeLock);
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
			pthread_mutex_unlock(&tmpList->nodeLock);
		}
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
//		count++;
		int key = rand() % KEY_RANGE;
		int hash = hashFunc(key, htable);
//		cout << "Searching for value " << key << "\n";
		List * tmpList = htable->table[hash];
		Node * tmpTail;
		iterations++;
		if(tmpList)
		{
			pthread_mutex_lock(&tmpList->nodeLock);
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
			pthread_mutex_unlock(&tmpList->nodeLock);
		}
		else 
		{
//			cout << "Null List for value " << key << "\n";
			pthread_mutex_lock(&listLock);
			nSearches++;
			pthread_mutex_unlock(&listLock);
		}
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
for(int k = 0; k < 20; k++){
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
