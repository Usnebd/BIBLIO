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
bool matchBook(Book_t* book, Book_t* bookNode);
bool isPresent(Book_t* book, Elem* head);
bool equalAuthors(Book_t* book, Book_t* bookNode);
void addBibToConf(char* name_bib);
void deleteFromConf(char* name_bib);
void dumpRecord(char* filename, Elem* head);
static void gestore (int signum);
