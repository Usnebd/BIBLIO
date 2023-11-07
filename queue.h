#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

typedef struct n{
	void* data;
	struct n * next;
} Node;

typedef struct {
	Node* head;	
	Node* tail;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

} Queue;

void init(Queue* q);

void destroy(Queue* q);

void push(void* data, Queue* q);

void* pop(Queue* q);
