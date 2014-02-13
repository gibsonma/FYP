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
#define NUM_THREADS 1
#define MAX_THREAD_VAL 8
#define KEY_RANGE 5
#define EXECUTION_TIME 1
#define PAUSE 2
#define MIN_DELAY 1
#define MAX_DELAY 10

//Modes of Operation - If neither defined then defaults to assembly spinlock
//#define LOCKED //Uses simple mutex blocking
//#define SPINLOCK //Uses a spinlock implemented with C++ atomics
//#define CASLOCK
//#define TAS
//#define TICKET

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
	
	pthread_mutex_lock(&listLock);
	int key = rand() % KEY_RANGE;
	cout << "Attempting to add " << key << "\n";
	Node * volatile tmpHead = head;
	Node * newNode = new Node(key, NULL);
	Node * volatile tmpNode = head;
	Node * volatile prevNode;
	if(tmpNode == NULL)//If linked list is empty
	{
		cout << "Linked list is empty \n";
		head = newNode;//Set head to be new node
		
	}
	else if(tmpNode->key > newNode->key)//If new key is smaller than head
	{
		cout << "New node is smaller than head \n";
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
			cout << "Node already present\n";
		}
		else
		{
			cout << "Adding Node\n";
			newNode->next = tmpNode;
			prevNode->next = newNode;
		}
		
	}
	printList();
	pthread_mutex_unlock(&listLock);
	
}

void* remove(void* threadid)
{
	pthread_mutex_lock(&listLock);
	int key = rand() % KEY_RANGE;
	cout << "Attempting to remove " << key << "\n";
	Node * tmpNode = head;
	Node * prevNode;
	if(tmpNode == NULL)//If linked list is empty then return
	{
		cout << "List is empty\n";
		
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
			cout << "Removing node" << key << "\n";
			prevNode->next = tmpNode->next;
		}
		else cout <<"Node does not exist\n";
	}
	printList();
	pthread_mutex_unlock(&listLock);
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
	//	gettimeofday(&stop_time, NULL);
	//	total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
	//	printf("%lld ,",iterations/EXECUTION_TIME);
//		printf("Total executing time %lld microseconds with %lld iterations per second and %d threads\n", total_time, iterations/EXECUTION_TIME, i);
		iterations = 0;
	}
	cout << "Final list: "; printList();

	return 0;
}
