/*
   A lockless hash table
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
#define KEY_RANGE 128
//#define KEY_RANGE 131072
//#define KEY_RANGE 134217728
#define INIT_TABLE_SIZE 128
//#define INIT_TABLE_SIZE 131072
//#define INIT_TABLE_SIZE 134217728
#define MAX_THREAD_VAL 128
#define EXECUTION_TIME 1
//#define DEBUG
using namespace std;
struct timeval start_time, stop_time;
long long total_time = 0, tsart = 0, tstop = 0;
std::atomic<int> iterations = ATOMIC_VAR_INIT(0);
struct Node{
	int key;
	Node * next;
	Node(int key) : key(key), next(nullptr){}
};
struct List{
	std::atomic<Node *> head;
	std::atomic<Node *> tail;
};
struct Table{
	int numItems;
	int size;
	List ** table;
	Table() 
	{
		size = INIT_TABLE_SIZE;
		table = new List*[size];
		for(int i = 0; i < size; i++)table[i] = NULL;
		numItems = 0;
	}
};
Table * htable = new Table();
int hashFunc(int item)
{
	return item % htable->size;
}


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
			cout << "List contained at index: " << i << "\n";
			while(tmpTail != NULL && tmpTail != tmpHead->next)
			{
		//		cout << tmpTail->key << " , ";
				count++;
				tmpTail = tmpTail->next;
			}
			
		//	if(currBucket->tail.load() == NULL)cout << "Tail : NULL ";
		//	else cout << "Tail : " << currBucket->tail.load()->key << " ";
		//	if(currBucket->head.load() == NULL)cout << "Head : NULL ";
		//	else cout << "Head : " << currBucket->head.load()->key << " ";
			cout << "Number of items in list: " << count << "\n";
			count = 0;
		}
	//	else cout << "Table index " << i << " empty\n";
	}
	cout << "\n";
}

void * add(void * threadid)
{
	while(true)
	{
		int key = rand() % KEY_RANGE;
		int hash = hashFunc(key);
//		cout << "Attempting to add " << key << " to index " << hash << "\n";
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
			if(std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, newNode) && std::atomic_compare_exchange_weak(&tmpList->tail, &tmpTail, newNode))
			{
//						cout << "New List created\n";
			}
		}
		tmpHead = tmpList->head.load();
		tmpTail = tmpList->tail.load();
		if((tmpHead == NULL || tmpTail == NULL) && tmpList == htable->table[hash])
		{
			if(std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, newNode) && std::atomic_compare_exchange_weak(&tmpList->tail, &tmpTail, newNode))
			{
//				cout << "List head & tail set\n";
			}
		}
		else if(tmpList == htable->table[hash] && tmpHead == tmpList->head.load()) 
		{
			if(std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, newNode))
			{
				tmpHead->next = newNode;
//				cout << "Node " << newNode->key << " added to list\n";
			}
		}
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

void * remove(void * threadid)
{
	while(true)
	{
		std::atomic_fetch_add(&iterations, 1);
		int hash = hashFunc(rand() % KEY_RANGE);
		Node * tmpTail;
		List * tmpList = htable->table[hash];
		if(tmpList != NULL)
		{
			tmpTail = tmpList->tail.load();
			if(tmpTail != NULL && tmpList == htable->table[hash])
			{
//				cout << "Attempting to remove the tail " << tmpTail->key << "\n";
				if(std::atomic_compare_exchange_weak(&tmpList->tail, &tmpTail, tmpTail->next))
				{
					if(tmpTail->next == NULL)
					{
//						cout << "List is now empty\n";
						Node * tmpHead = tmpList->head.load();
						while(!std::atomic_compare_exchange_weak(&tmpList->head, &tmpHead, tmpTail->next));
//						cout << "Head set to null\n";
					}
//					else cout << "New tail is " << tmpTail->next->key << "\n";
		//			printTable();
				}
			}
		}
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
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
		//      printf("Total executing time %lld microseconds, %lld iterations/s  and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}
#if defined(DEBUG)
	printTable();
#endif
	return 0;
}

