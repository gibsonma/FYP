/* 
   Based on the lockless linked list in list_lockless.cpp
   This is in essence a lockless buffer with a head and a tail
   Threads continually add onto and remove from the head and the tail
   Unlike in list_lockless.cpp, duplicates are allowed and the list is not sorted
   Note, this is different to mpmc_lockless because the head and tail in this implementation
   are switched, with the tail being the leftmost part of the list and the head being the rightmost
   point in the list. In addition this is a singely linked list as a result.
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
#define MAX_THREAD_VAL 128
#define EXECUTION_TIME 1
#define DEBUG
using namespace std;
struct timeval start_time, stop_time;
long long total_time = 0, tsart = 0, tstop = 0;
std::atomic<int> iterations = ATOMIC_VAR_INIT(0);
struct Node{

	int key;
	Node * next;
	Node(int key) : key(key), next(nullptr){}
};
std::atomic<Node *> head;
std::atomic<Node *> tail;

void printList()
{
	Node * tmpNode = tail.load();
	long long count = 0;
//	cout << "List: ";
	while(tmpNode != NULL)
	{
		//cout << tmpNode->key << ",";
		tmpNode = tmpNode->next;
		count++;
	}
	cout << "\nTotal number of nodes: " << count << "\n";
}
void * add(void * threadid)
{
	while(true)
	{
		int key = rand() % KEY_RANGE;
		std::atomic_fetch_add(&iterations, 1);
		//cout << "Attempting to add " << key << "\n";
		Node * newNode = new Node(key);
		Node * tmpHead = head.load();
		Node * tmpTail = tail.load();
		if(tmpHead == NULL || tmpTail == NULL)//If list is empty
		{
			//cout << "Linked list is empty\n";
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, newNode) && std::atomic_compare_exchange_weak(&tail, &tmpTail, newNode))
			{
				//cout << "List head added and tail added\n";
				//printList();
			}
		}
		else
		{
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, newNode))
			{
				tmpHead->next = newNode;
				//cout << "New node added successfully\n";
				//printList();
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
		Node * tmpTail = tail.load();
		Node * tmpNode;
		Node * tmpHead;
		if(tmpTail != NULL)
		{
			//cout << "Attempting to remove the tail " << tmpTail->key << "\n";
			if(std::atomic_compare_exchange_weak(&tail, &tmpTail, tmpTail->next))
			{
				if(tmpTail->next == NULL)//If tail is now null then head must also be set 
				{			 //to null
					//cout << "List now empty\n";
					tmpHead = head.load();
					while(!std::atomic_compare_exchange_weak(&head, &tmpHead, tmpTail->next));
					//cout << "Head set to null\n";
				}
				//else cout << "New tail is " << tmpTail->next->key << "\n";
				//printList();
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
		printf("%lld ,",iterations/EXECUTION_TIME);
		//      printf("Total executing time %lld microseconds, %lld iterations/s  and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}
#if defined(DEBUG)
	printList();
#endif
	return 0;
}

