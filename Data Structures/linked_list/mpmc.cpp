//Based on the lockless linked list in test.cpp
//This is in essence a lockless buffer with a head and a tail
//Threads continually add onto and remove from the head and the tail
//Unlike in test.cpp, duplicates are allowed and the list is not sorted
//Note, that due to an issue with seg faults, nodes are not actually
//deleted but instead have their keys set to 0
#include <atomic>
#include <iostream>
#include "pthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#define KEY_RANGE 128
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
	Node * prev;
	Node(int key) : key(key), next(nullptr), prev(nullptr){}
};
std::atomic<Node *> head;
std::atomic<Node *> tail;

void printList()
{
	Node * tmpNode = head;
	long long count = -1;
	while(tmpNode != NULL)
	{
		cout << tmpNode->key << ",";
		tmpNode = tmpNode->next;
	}
	cout << "\n";
}
void * add(void * threadid)
{
	while(true)
	{
		int key = rand() % KEY_RANGE;
		std::atomic_fetch_add(&iterations, 1);
	//	cout << "Attempting to add " << key << "\n";
		Node * newNode = new Node(key);
		Node * tmpHead = head.load();
		Node * tmpTail = tail.load();
		if(tmpHead == NULL)//If list is empty
		{
			//cout << "Linked list is empty\n";
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, newNode) && std::atomic_compare_exchange_weak(&tail, &tmpTail, newNode))
			{
	//			cout << "List head added and tail added\n";
			}
		}
		else
		{
			newNode->next = tmpHead;
			tmpHead->prev = newNode;
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, newNode))
			{
	//			cout << "New node added successfully\n";
//				printList();
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
		Node * tmpHead = head.load();
		Node * tmpNode;
		if(tmpTail != NULL)
		{
			tmpNode = tmpTail->prev;
			tmpTail->key = 0;
			if(std::atomic_compare_exchange_weak(&tail, &tmpTail, tmpNode))
			{
			//	cout << "New tail is " << tmpNode->key << "\n";
//				printList();
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
	cout << "Final list: "; printList();
#endif
	return 0;
}

