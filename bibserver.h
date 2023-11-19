#include<time.h>
#include<stddef.h>
#include<signal.h>
#include <sys/file.h>
#include"structures.h"
#include"bookToRecord.h"
#include"freeBook.h"
#include"unboundedqueue.h"

int aggiornaMax(fd_set set, int max );
int countAttributes(char* str);
Book_t* recordToBook(char* riga, Book_t* book);
void* worker(void* args);
bool matchElemBook(Book_t* book, Book_t* bookNode);
bool isAlreadyPresent(Book_t* book, Elem* head);
bool equalAuthors(Book_t* book, Book_t* bookNode);
void addBibToConf(char* name_bib);
int nullFieldsCount(Book_t* book);
void deleteFromConf(char* name_bib);
static void gestore (int signum);