#include<string.h>
#include"structures.h"

void bookToRecord(Book_t* book, char* data,char msgType){
    if(book->autore!=NULL){
        if(book->autore->val!=NULL){
            strcpy(data, "autore: ");
            strcat(data, book->autore->val);
            strcat(data, ";");
            while(book->autore!=NULL){
                strcat(data, " autore: ");
                strcat(data, book->autore->val);
                strcat(data, ";");
                book->autore=book->autore->next;
            }
        }
    }
    if((book->titolo)!=NULL){
        strcat(data," titolo: ");
        strcat(data,book->titolo);
        strcat(data, ";");
    }
    if((book->editore)!=NULL){
        strcat(data," editore: ");
        strcat(data,book->editore);
        strcat(data, ";");
    }
    if((book->anno)!=0){
        strcat(data,"anno: ");
        data[strlen(data)]=book->anno;
        strcat(data,";");
    }
    if((book->nota)!=NULL){
        strcat(data," nota: ");
        strcat(data,book->nota);
        strcat(data, ";");
    }
    if((book->collocazione)!=NULL){
        strcat(data," collocazione: ");
        strcat(data,book->collocazione);
        strcat(data, ";");
    }
    if((book->luogo_pubblicazione)!=NULL){
        strcat(data," luogo_pubblicazione: ");
        strcat(data,book->luogo_pubblicazione);
        strcat(data, ";");
    }
    if((book->descrizione_fisica)!=NULL){
        strcat(data," descrizione_fisica: ");
        strcat(data,book->descrizione_fisica);
        strcat(data, ";");
    }
    if((book->prestito)!=NULL && (msgType=='Q')){
        strcat(data," prestito: ");
        strcat(data,book->prestito);
        strcat(data, ";");
    }
}