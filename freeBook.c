#include"structures.h"

void freeBook(Book_t* book){
    if(book->autore!=NULL){
        NodoAutore* currAuthor=book->autore;
        while(currAuthor!=NULL){
            if(currAuthor->val!=NULL){
                free(currAuthor->val);
                currAuthor=currAuthor->next;
                if(book->autore->next!=NULL){
                    free(book->autore->next);
                }
            }
            free(book->autore);
        }
    }
    if(book->nota!=NULL){
        free(book->nota);
    }
    if(book->collocazione!=NULL){
        free(book->collocazione);
    }
    if(book->descrizione_fisica!=NULL){
        free(book->descrizione_fisica);
    }
    if(book->editore!=NULL){
        free(book->editore);
    }
    if(book->luogo_pubblicazione!=NULL){
        free(book->luogo_pubblicazione);
    }
    if(book->titolo!=NULL){
        free(book->titolo);
    }
    free(book);
}