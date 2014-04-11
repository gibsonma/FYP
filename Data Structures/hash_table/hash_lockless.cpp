/*
   A lockless hash table
 */
#include <atomic>
#include <iostream>
#include "pthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "xmmintrin.h"
#include "median.h"
#define KEY_RANGE 128
//#define KEY_RANGE 131072
//#define KEY_RANGE 134217728
#define INIT_TABLE_SIZE 128
//#define INIT_TABLE_SIZE 131072
//#define INIT_TABLE_SIZE 134217728
#define MAX_THREAD_VAL 128
#define EXECUTION_TIME 1
//#define DEBUG
//#define COUNTS
//#define RESIZE
#define MAX_LIST_LENGTH 10
using namespace std;
pthread_mutex_t hLock = PTHREAD_MUTEX_INITIALIZER;
struct timeval start_time, stop_time;
long long total_time = 0, tsart = 0, tstop = 0;
std::atomic<int> iterations = ATOMIC_VAR_INIT(0);
std::atomic<int> pSearches = ATOMIC_VAR_INIT(0);
std::atomic<int> nSearches = ATOMIC_VAR_INIT(0);
std::atomic<int> containsC = ATOMIC_VAR_INIT(0);
std::atomic<int> addC = ATOMIC_VAR_INIT(0);
std::atomic<int> removeC = ATOMIC_VAR_INIT(0);
long long results[8];
struct Node{
	int key;
	Node * next;
	Node(int key) : key(key), next(nullptr){}
};
struct List{
	std::atomic<Node *> head;
	std::atomic<Node *> tail;
	int listLength = 0;
	void add(int key);
};
struct Table{
	int numItems;
	int size;
	List ** table;
	Table(int tSize) 
	{
		size = tSize;
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
void List::add(int key)
{
	Node * newNode = new Node(key);
	Node * tmpTail = tail.load();
	Node * tmpHead = head.load();
	if(tmpHead == NULL || tmpTail == NULL)
	{
		head.store(newNode);
		tail.store(newNode);
	}
	else if(tmpHead == head.load())
	{
		tmpHead->next = newNode;
		head.store(newNode);
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
			tmpTail = currBucket->tail.load();
			tmpHead = currBucket->head.load();
			while(tmpTail != NULL && tmpTail != tmpHead->next)
			{
				count++;
				tmpTail = tmpTail->next;
			}
			if(count > 0)cout << "List: " << i << " Number of items in list: " << count << "\n";
			count = 0;
		}
	}
	cout << "\n";
}
#if defined(COUNTS)
void printSearchResults()
{
	cout << "Total Searches: " << (pSearches+nSearches) << " Positive Searches: " << pSearches << " Negative Searches: " << nSearches << "\n";
}
void printTCounts()
{
	cout << "Contains: " << containsC << " Adds: " << addC << " Removes: " << removeC << "\n";
}
#endif
void * add(void * threadid)
{
	while(true)
	{
		int key = rand() % KEY_RANGE;
		int hash = hashFunc(key, htable);
		Node * newNode = new Node(key);
		Node * tmpHead;
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		std::atomic_fetch_add(&iterations, 1);
		if(tmpList == NULL)//If index in table is empty
		{
			htable->table[hash] = new List();
			tmpList = htable->table[hash];
			tmpHead = tmpList->head.load();
			tmpTail = tmpList->tail.load();
			while(true){
				if(std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, newNode) && std::atomic_compare_exchange_weak(&tmpList->tail, &tmpTail, newNode))
				{
					tmpList->listLength = 1;
					break;
				}
			}
		}
		tmpHead = tmpList->head.load();
		tmpTail = tmpList->tail.load();
		if((tmpHead == NULL || tmpTail == NULL) && tmpList == htable->table[hash])
		{
			while(true){
				if(std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, newNode) && std::atomic_compare_exchange_weak(&tmpList->tail, &tmpTail, newNode))
				{
					tmpList->listLength = 1;
					break;
				}
			}
		}
		else if(tmpList == htable->table[hash] && tmpHead == tmpList->head.load()) 
		{
			while(true){
				if(std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, newNode))
				{
					tmpHead->next = newNode;
					tmpList->listLength++;
					break;
				}
			}
		}
#if defined(RESIZE)
		pthread_mutex_lock(&hLock);
		if(tmpList->listLength >= MAX_LIST_LENGTH)
		{
			sleep(2);
			htable = resize(htable->size);
		}
		pthread_mutex_unlock(&hLock);
#endif
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void * remove(void * threadid)
{
	while(true)
	{
		std::atomic_fetch_add(&iterations, 1);
		int hash = hashFunc(rand() % KEY_RANGE, htable);
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		if(tmpList != NULL)
		{
			tmpTail = tmpList->tail.load();
			if(tmpTail != NULL && tmpList == htable->table[hash])
			{
				while(true){
					if(std::atomic_compare_exchange_weak(&tmpList->tail, &tmpTail, tmpTail->next))
					{
						if(tmpTail->next == NULL)
						{
							tmpList->listLength = 0;
							Node * tmpHead = tmpList->head.load();
							while(!std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, tmpTail->next));
						}
						tmpList->listLength--;
						break;
					}
				}
			}
		}
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}
void * contains(void * threadid)
{
	while(true)
	{
		std::atomic_fetch_add(&iterations, 1);
		int key = rand() % KEY_RANGE;
		int hash = hashFunc(key, htable);
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		if(tmpList != NULL)
		{
			tmpTail = tmpList->tail.load();
			while(tmpTail != NULL && tmpList == htable->table[hash])
			{
				if(tmpTail->key == key)
				{
					std::atomic_fetch_add(&pSearches, 1);
					break;
				}
				tmpTail = tmpTail->next;
			}
			if(tmpTail == NULL)std::atomic_fetch_add(&nSearches, 1);
		}
		else std::atomic_fetch_add(&nSearches, 1);
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;

	}
}
void * choose(void * threadid)
{
	int num = rand() % 128;
	if(num >= 12)
	{
#if defined(COUNTS)
		std::atomic_fetch_add(&containsC, 1);
#endif		
		contains(threadid);
	}
	else if(num >= 2)
	{
#if defined(COUNTS)
		std::atomic_fetch_add(&addC, 1);
#endif
		add(threadid);
	}
	else 
	{
#if defined(COUNTS)
		std::atomic_fetch_add(&removeC, 1);
#endif
		remove(threadid);
	}
}

int main()
{
	for(int i = 1; i <= MAX_THREAD_VAL; i = i * 2){
		for(int j = 0; j < 7; j++){	
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
			//printf("%lld \n",iterations/EXECUTION_TIME);
			//			cout << iterations/EXECUTION_TIME << "\n"; 
			results[j] = iterations/EXECUTION_TIME;		
			iterations = 0;
		}
		getMedian(results, 8);
	}
	cout << "\n";
#if defined(DEBUG)
	printTable();
#endif
#if defined(RESIZE)
	cout << "Final Size: " << htable->size << "\n";
#endif
#if defined(COUNTS)
	printSearchResults();
	printTCounts();
#endif
	return 0;
}

