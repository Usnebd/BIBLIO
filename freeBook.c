#include"structures.h"

void freeBook(Book_t* book){
    if(book->autore!=NULL){
        NodoAutore* currAuthor=book->autore;
        while(currAuthor!=NULL){
            if(currAuthor->val!=NULL){
                free(currAuthor->val);
                NodoAutore* tmp=currAuthor;
                currAuthor=currAuthor->next;
                free(tmp);
            }
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
    if(book->volume!=NULL){
        free(book->volume);
    }
    if(book->scaffale!=NULL){
        free(book->scaffale);
    }
    if(book->luogo_pubblicazione!=NULL){
        free(book->luogo_pubblicazione);
    }
    if(book->titolo!=NULL){
        free(book->titolo);
    }
    free(book);
}