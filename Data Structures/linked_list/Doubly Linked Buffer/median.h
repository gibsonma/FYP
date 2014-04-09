#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
void getMedian(long long results[], int size)
{
	long long* sortedResults = new long long[size];
	for (int i = 0; i < size; ++i) {
		sortedResults[i] = results[i];
	}
	for (int i = size - 1; i > 0; --i) {
		for (int j = 0; j < i; ++j) {
			if (sortedResults[j] > sortedResults[j+1]) {
				long long tmp = sortedResults[j];
				sortedResults[j] = sortedResults[j+1];
				sortedResults[j+1] = tmp;
			}
		}
	}
	long median = 0.0;
	if ((size % 2) == 0) {
		median = (sortedResults[size/2] + sortedResults[(size/2) - 1])/2.0;
	} else {
		median = sortedResults[size/2];
	}
	cout << median << ",";
}

