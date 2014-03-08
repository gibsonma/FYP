#include <atomic>
#include <iostream>
#include "pthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
//#define KEY_RANGE 128
//#define KEY_RANGE 131072
#define KEY_RANGE 134217728
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

void printList()
{
	Node * tmpNode = head;
	bool flag = true;
	long long count = -1;
//	cout << "List:";
	while(tmpNode != NULL)
	{
//		cout << tmpNode->key << ",";
		if(tmpNode->key <= count)flag = false;
		count = tmpNode->key;
		tmpNode = tmpNode->next;
	}
	if(!flag)cout << "ERROR IN LIST";
	cout << "\n";
}

void * add(void * threadid)
{
	while(true)
	{
		int key = rand() % KEY_RANGE;
		std::atomic_fetch_add(&iterations, 1);
//		cout << "Attempting to add " << key << "\n";	
		Node * newNode = new Node(key);
		Node * tmpHead = head.load();
		Node * prevNode;
		if(tmpHead == NULL)
		{
//			cout << "Linked list is empty \n";
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, newNode))//Obj, exp, desire
			{
//				cout << "List head added \n";
			}
		}
		else if(tmpHead->key > newNode->key)
		{
	//		cout << "New node is smaller than head \n";
			newNode->next = tmpHead;
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, newNode))
			{
	//			cout << "List head replaced \n";
			}
		}
		else
		{
			while(tmpHead && (tmpHead->key < newNode->key))
			{
				prevNode = tmpHead;
				tmpHead = tmpHead->next;
			}
			if(tmpHead && (tmpHead->key == newNode->key))
			{
				//cout << "Node already present \n";
			}
			else
			{
				newNode->next = tmpHead;
				prevNode->next = newNode;
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
		int key = rand() % KEY_RANGE;
		std::atomic_fetch_add(&iterations, 1);
//		cout << "Attempting to Remove " << key << "\n";	
		Node * tmpHead = head.load();
		Node * prevNode;
		if(tmpHead == NULL)
		{
//			cout << "Linked list is empty \n";
		}
		else if(tmpHead->key == key)
		{
			if(std::atomic_compare_exchange_weak(&head, &tmpHead, tmpHead->next))
			{
				//cout << "Head reassigned";
				delete tmpHead;
			}
		}
		else
		{
			while(tmpHead && (tmpHead->key != key))
			{
				prevNode = tmpHead;
				tmpHead = tmpHead->next;
			}
			if(tmpHead && (tmpHead->key == key))
			{
				//cout << "Removing Node " << key << "\n";
				prevNode->next = tmpHead->next;
			}
			else {}//cout << "Node does not exist \n";
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
	//	printf("Total executing time %lld microseconds, %lld iterations/s  and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}
#if defined(DEBUG)
	cout << "Final list: "; printList();
#endif
	return 0;
}

