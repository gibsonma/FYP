#include <atomic>
#include <iostream>

using namespace std;

struct Node{
	
		int key;
		Node * next;
		Node(int key) : key(key), next(nullptr){}
};
int main()
{
	std::atomic<Node *> head;
	Node * newNode = head.load();
	while(!std::atomic_compare_exchange_weak(&head, &newNode->next, newNode))
	{//(obj, expected, desired)
	}
	return 0;
}
