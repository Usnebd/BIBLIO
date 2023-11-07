#include "queue.h"


void init(Queue* q){
	q->head=q->tail=NULL;
	pthread_mutex_init(&q->mutex,NULL);
	pthread_cond_init(&q->cond,NULL);
	
}

void destroy(Queue* q){
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->cond);
	while(q->head){
		Node* temp =q->head;
		q->head=q->head->next;
	}
	q->tail=NULL;
}

void push(void* data, Queue* q){
	Node* n=(Node*)malloc(sizeof(Node));
	n->data=data;
	n->next=NULL;

	pthread_mutex_lock(&q->mutex);
	if(q->tail==NULL){
		q->head=n;
	}
	else{
		q->tail->next=n;
	}
	q->tail=n;
	
	pthread_cond_signal(&q->cond);
	pthread_mutex_unlock(&q->mutex);
}

void* pop(Queue* q){
	
	pthread_mutex_lock(&q->mutex);
	while(q->head==NULL)
		pthread_cond_wait(&q->cond,&q->mutex);
		
	Node* n=q->head;

	q->head=q->head->next;
	if(q->head==NULL)
		q->tail=NULL;
	pthread_mutex_unlock(&q->mutex);
	
	void* data=n->data;
	free(n);
	return data;
}
