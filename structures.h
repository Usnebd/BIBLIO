#define MSG_QUERY 'Q'
#define MSG_LOAN 'L'
#define MSG_RECORD 'R'
#define MSG_NO 'N'
#define MSG_ERROR 'E'
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#include<sys/select.h>
#include "unboundedqueue.h"

struct
{
    char* titolo;
    char* editore;
    int anno;
    char* nota;
    char* collocazione;
    char* luogo_pubblicazione;
    char* descrizione_fisica;  
    char prestito[19];
    struct z* autore;
}typedef Book_t;

typedef enum { false, true } bool;

struct k{
    Book_t* val;
    struct k* next;
};
typedef struct k Elem;

struct z{
    char* val;
    struct z* next;
};
typedef struct z NodoAutore;

typedef struct {
	Queue_t* q;
    Elem* list;
    pthread_mutex_t* mutex;
    FILE* flog;
}
arg_t;