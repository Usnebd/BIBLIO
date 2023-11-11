#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#include<sys/select.h>
#include "structures.h"
#include "unboundedqueue.h"

int countAttributes(char* str);
Book* get_record(char* riga, Book* book);
void* worker(void* args);