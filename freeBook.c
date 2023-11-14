#include"structures.h"

void freeBook(Book_t* book){
    if(book->autore!=NULL){
        NodoAutore* nodo=book->autore;
        while(nodo!=NULL){
            if(nodo->val!=NULL){
                free(book->autore->val);
            }
            book->autore=book->autore->next;
            free(nodo);
        }
        nodo=book->autore;
        book->autore=book->autore->next;
        free(nodo);
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