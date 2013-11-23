/* Code adapted from http://en.wikipedia.org/wiki/Circular_buffer
 * Spin lock code taken from http://locklessinc.com
 * Circular buffer example, keeps one slot open 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#define TIMING
//#define LOCKED
#define LOCKLESS
#define barrier() asm volatile("": : :"memory")
#define cpu_relax() asm volatile("pause\n": : :"memory")
#define EBUSY 1

#ifdef LOCKLESS
static inline unsigned xchg_32(void *ptr, unsigned x)
{
	__asm__ __volatile__("xchgl %0,%1"
			:"=r" ((unsigned) x)
			:"m" (*(volatile unsigned *)ptr), "0" (x)
			:"memory");

	return x;
}
typedef unsigned spinlock;
spinlock spinLock;
static void spin_lock(spinlock *lock)
{
	while (1)
	{
		if (!xchg_32(lock, EBUSY)) return;

		while (*lock) cpu_relax();
	}
}

static void spin_unlock(spinlock *lock)
{
	barrier();
	*lock = 0;
}
#endif
#ifdef TIMING
struct timeval start_time;
struct timeval stop_time;
long long total_time = 0;
long long tstart = 0,  tstop = 0;
#endif

pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;
const int NUM_THREADS = 8;
const int sizeOfBuffer = 10;
const int EXECUTION_TIME = 1;
volatile long long iterations = 0;

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
	while(1)
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

#ifdef LOCKLESS
		spin_lock(&spinLock);
		printf("Value: %d placed in index: %d\n", count.value + 1, cb.end);
		count.value++;
		cb.elems[cb.end] = count;
		cb.end = (cb.end + 1) % cb.size;
		if (cb.end == cb.start)
			cb.start = (cb.start + 1) % cb.size; /* full, overwrite */
		iterations++;
		spin_unlock(&spinLock);
#endif
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
	}
}

/* Read oldest element. App must ensure !cbIsEmpty() first. */
void *cbRead(void * threadid) {
	while(1)
	{
#ifdef LOCKED
		pthread_mutex_lock(&bufferLock);
		tmp = cb.elems[cb.start];
		printf("Value: %d read from index: %d\n", tmp.value, cb.start);
		cb.start = (cb.start + 1) % cb.size;
		pthread_mutex_unlock(&bufferLock);
#endif

#ifdef LOCKLESS
		spin_lock(&spinLock);
		tmp = cb.elems[cb.start];
		printf("Value: %d read from index: %d\n", tmp.value, cb.start);
		cb.start = (cb.start + 1) % cb.size;
		spin_unlock(&spinLock);
#endif
		gettimeofday(&stop_time, NULL);
		if(((stop_time.tv_sec + (stop_time.tv_usec/1000000.0)) -( start_time.tv_sec + (start_time.tv_usec/1000000.0))) > EXECUTION_TIME) break;
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
	printf("Total executing time %lld microseconds with %lld iterations per second\n", total_time, iterations/EXECUTION_TIME);
#endif
	return 0;
}
