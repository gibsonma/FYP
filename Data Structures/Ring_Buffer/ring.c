/* Code adapted from http://en.wikipedia.org/wiki/Circular_buffer
 * Circular buffer example, keeps one slot open 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#define TIMING
#define LOCKED
//#define LOCKLESS

#ifdef TIMING
struct timeval start_time;
struct timeval stop_time;
long long total_time = 0;
#endif

pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;
const long long MAX_ITERATIONS = 40;
const int NUM_THREADS = 4;
const int sizeOfBuffer = 10;
long long iterations = 0;

/* Opaque buffer element type.  This would be defined by the application. */
typedef struct { int value; } ElemType;

/* Circular buffer object */
typedef struct {
	int         size;   /* maximum number of elements           */
	int         start;  /* index of oldest element              */
	int         end;    /* index at which to write new element  */
	ElemType   *elems;  /* vector of elements                   */
} CircularBuffer;

void cbInit(CircularBuffer *cb, const int size);
CircularBuffer cb;
ElemType elem = {0}, count = {1}, tmp = {0};

void cbInit(CircularBuffer *cb, int size) {
	cb->size  = size + 1; /* include empty elem */
	cb->start = 0;
	cb->end   = 0;
	cb->elems = (ElemType *)calloc(cb->size, sizeof(ElemType));
}

void cbFree(CircularBuffer *cb) {
	free(cb->elems); /* OK if null */ 
}

int cbIsFull(CircularBuffer *cb) {
	return (cb->end + 1) % cb->size == cb->start; 
}

int cbIsEmpty(CircularBuffer *cb) {
	return cb->end == cb->start; 
}

/* Write an element, overwriting oldest element if buffer is full. App can
   choose to avoid the overwrite by checking cbIsFull(). */
void *cbWrite(void * threadid) {
	while(iterations < MAX_ITERATIONS)
	{
#ifdef LOCKED		
		pthread_mutex_lock(&bufferLock);
		printf("Value: %d placed in index: %d\n", count.value + 1, cb.end);
		count.value++;
		cb.elems[cb.end] = count;
		cb.end = (cb.end + 1) % cb.size;
		if (cb.end == cb.start)
			cb.start = (cb.start + 1) % cb.size; /* full, overwrite */
		iterations++;
		pthread_mutex_unlock(&bufferLock);
#endif	
	}
}

/* Read oldest element. App must ensure !cbIsEmpty() first. */
void *cbRead(void * threadid) {
	while(iterations < MAX_ITERATIONS)
	{
#ifdef LOCKED
		pthread_mutex_lock(&bufferLock);
		tmp = cb.elems[cb.start];
		printf("Value: %d read from index: %d\n", tmp.value, cb.start);
		cb.start = (cb.start + 1) % cb.size;
		pthread_mutex_unlock(&bufferLock);
#endif
	}
}

int main(int argc, char **argv) 
{
#ifdef TIMING
	gettimeofday(&start_time, NULL);
#endif
	cbInit(&cb, sizeOfBuffer);
	int t, rc;
	pthread_t threads[NUM_THREADS];
	for(t = 0; t < NUM_THREADS; t++)
	{
		printf("Creating Thread %d\n", t);
		if(t % 2 == 0)rc = pthread_create(&threads[t], NULL, cbWrite, (void *) t);
		else rc = pthread_create(&threads[t], NULL, cbRead, (void *) t);
	}
	for(t = 0; t < NUM_THREADS; t++)
	{
		pthread_join(threads[t], NULL);
	}
	cbFree(&cb);
#ifdef TIMING
	gettimeofday(&stop_time, NULL);
	total_time += (stop_time.tv_sec - start_time.tv_sec) * 1000000L + (stop_time.tv_usec - start_time.tv_usec);
	printf("Total executing time %lld microseconds with %lld iterations\n", total_time, MAX_ITERATIONS);
#endif
	return 0;
}
