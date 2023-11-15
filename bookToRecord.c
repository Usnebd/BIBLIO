#include<string.h>
#include"structures.h"

int bookToRecord(Book_t* book, char* data){
    bool notFirstIteration=false;
    if(book->autore!=NULL){
        if(book->autore->val!=NULL){
            strcpy(data, "autore: ");
            strcat(data, book->autore->val);
            strcat(data, ";");
            book->autore=book->autore->next;
            while(book->autore!=NULL){
                notFirstIteration=true;
                strcat(data," ");
                strcat(data, "autore: ");
                strcat(data, book->autore->val);
                strcat(data, ";");
                book->autore=book->autore->next;
            }
        }
    }
    if((book->titolo)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"titolo: ");
        strcat(data,book->titolo);
        strcat(data, ";");
    }
    if((book->editore)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"editore: ");
        strcat(data,book->editore);
        strcat(data, ";");
    }
    if((book->anno)!=0){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        sprintf(data+strlen(data), "anno: %d;", book->anno);
    }
    if((book->nota)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"nota: ");
        strcat(data,book->nota);
        strcat(data, ";");
    }
    if((book->collocazione)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"collocazione: ");
        strcat(data,book->collocazione);
        strcat(data, ";");
    }
    if((book->luogo_pubblicazione)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"luogo_pubblicazione: ");
        strcat(data,book->luogo_pubblicazione);
        strcat(data, ";");
    }
    if((book->descrizione_fisica)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"descrizione_fisica: ");
        strcat(data,book->descrizione_fisica);
        strcat(data, ";");
    }
    if(strcmp(book->prestito,"")!=0){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"prestito: ");
        strcat(data,book->prestito);
        strcat(data, ";");
    }
    return strlen(data);
}