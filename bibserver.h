#include<time.h>
#include"structures.h"
#include"bookToRecord.h"
#include"freeBook.h"
#include"unboundedqueue.h"
#define SIZE 400
#define UNIX_PATH_MAX 80
#define N 100

void aggiornaMax(fd_set set, int* max );
int countAttributes(char* str);
Book_t* recordToBook(char* riga, Book_t* book);
void* worker(void* args);
bool matchElemBook(Book_t* book, Book_t* bookNode);
void checkFromConf(char* name_bib);
void deleteFromConf(char* name_bib);