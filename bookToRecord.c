#include<string.h>
#include"structures.h"

int bookToRecord(Book_t* book, char* data){     //prende il riferimento ad un libro e una stringa vuota dove salvare il risultato
    bool notFirstIteration=false;             
    NodoAutore* nodeAuthor=book->autore;
    if(nodeAuthor!=NULL){
        if(nodeAuthor->val!=NULL){
            strcpy(data, "autore: ");
            strcat(data, nodeAuthor->val);
            strcat(data, ";");
            nodeAuthor=nodeAuthor->next;
            notFirstIteration=true;
            while(nodeAuthor!=NULL){
                strcat(data," ");
                strcat(data, "autore: ");
                strcat(data, nodeAuthor->val);
                strcat(data, ";");
                nodeAuthor=nodeAuthor->next;
            }
        }
    }
    if((book->titolo)!=NULL){               //se il campo titolo non è nullo
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"titolo: ");            //aggiungo alla stringa
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
    if((book->volume)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"volume: ");
        strcat(data,book->volume);
        strcat(data, ";");
    }
    if((book->scaffale)!=NULL){
        if(notFirstIteration==true){
            strcat(data," ");
        }else{
            notFirstIteration=true;
        }
        strcat(data,"scaffale: ");
        strcat(data,book->scaffale);
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
    return strlen(data);        //restituisco la lunghezza della stringa
}