#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#include <errno.h>
#include"structures.h"
#include"bookToRecord.h"
#include"freeBook.h"
#define SIZE 100
#define BUFFSIZE 1024 

int main(int argc, char* argv[]){
    Book_t* bookQuery=(Book_t*)malloc(sizeof(Book_t));      //definisco il libro e alloco memoria
    char type='Q';                                          //imposto di default il type del messaggio a "Q"
    for(int i=1;i<argc;i++){                                //scorro tutti gli argomenti passati a bibclient 
        if(strncmp(argv[i],"--autore=",9)==0){      
            bookQuery->autore=(NodoAutore*)malloc(sizeof(NodoAutore));
            bookQuery->autore->val=(char*)malloc(strlen(argv[i]+9));
            strcpy(bookQuery->autore->val,argv[i]+9);
        }else if(strncmp(argv[i], "--titolo=",9)==0){       //se argv[i] è "--titolo=" allora inserisco nel libro il valore
            bookQuery->titolo=(char*)malloc(strlen(argv[i]+9));
            strcpy(bookQuery->titolo,argv[i]+9);
        }else if(strncmp(argv[i], "--volume=",9)==0){
            bookQuery->volume=(char*)malloc(strlen(argv[i]+9));
            strcpy(bookQuery->titolo,argv[i]+9);
        }else if(strncmp(argv[i], "--editore=",10)==0){
            bookQuery->editore=(char*)malloc(strlen(argv[i]+10));
            strcpy(bookQuery->editore,argv[i]+10);
        }else if(strncmp(argv[i], "--nota=",7)==0){
            bookQuery->nota=(char*)malloc(strlen(argv[i]+7));
            strcpy(bookQuery->nota,argv[i]+7);
        }else if(strncmp(argv[i], "--collocazione=",15)==0){
            bookQuery->collocazione=(char*)malloc(strlen(argv[i]+15));
            strcpy(bookQuery->collocazione,argv[i]+15);
        }else if(strncmp(argv[i], "--luogo_pubblicazione=",22)==0){
            bookQuery->luogo_pubblicazione=(char*)malloc(strlen(argv[i]+22));
            strcpy(bookQuery->luogo_pubblicazione,argv[i]+22);
        }else if(strncmp(argv[i], "--scaffale=",11)==0){
            bookQuery->scaffale=(char*)malloc(strlen(argv[i]+11));
            strcpy(bookQuery->scaffale,argv[i]+11);
        }else if(strncmp(argv[i], "--anno=",7)==0){
            bookQuery->anno=atoi(argv[i]+7);
        }else if(strncmp(argv[i], "--descrizione_fisica=",21)==0){
            bookQuery->descrizione_fisica=(char*)malloc(strlen(argv[i]+21));
            strcpy(bookQuery->descrizione_fisica,argv[i]+21);
        }else if(strcmp(argv[i], "-p")==0){         //se c'è l'opzione -p allora setto type="L"
            type='L';
        }else{                                      //se non è nessuno dei precedenti allora è sbagliato il formato
            printf("Wrong format of args\n");
            exit(EXIT_FAILURE);                     //esce
        }
    }
    char buffer[BUFFSIZE];                          //creo un buffer
    memset(buffer,0,BUFFSIZE);                      //azzero il buffer
    unsigned int messagelength=bookToRecord(bookQuery,buffer)+1;    //passo il buffer e il riferimento al libro a bookToRecord
    char* message=(char*)malloc(BUFFSIZE);
    memcpy(&message[0], &type, 1);                                      //copio il type nel primo byte del messaggio
    memcpy(&message[1], &messagelength, sizeof(unsigned int));          // Copia la lunghezza della struct nel buffer
    memcpy(&message[1+sizeof(messagelength)], buffer, messagelength);   // Copia i dati della struct nel buffer
    size_t size=SIZE;
    struct sockaddr_un sa;
    sa.sun_family=AF_UNIX;
    bool emptyConf=true;
    FILE* fconf=fopen("bib.conf","r");              //apro il file bib.conf in lettura
    if(fconf){
        char* sockname;
        char* riga=(char*)malloc(SIZE);
        int nchar;
        while((nchar=getline(&riga,&size,fconf))!=-1){      //scorro le righe del file contententi gli indirizzia ai vari server
            if(nchar!=0 && nchar!=1){
                emptyConf=false;
                strtok(riga,",");
                strtok(riga,":");
                sockname=strtok(NULL,":");
                if(strspn(sockname, "X") != strlen(sockname)){ //se la riga è BibName:XXXXX,Sockpath:XXXXX  allora nome invalido
                    char path[1+strlen(sockname)];             //un server era attivo ma poi è andato offline
                    strcpy(path,"./");
                    strcat(path,strtok(sockname,":"));
                    strcpy(sa.sun_path,path);
                    int serverSocket;
                    checkSyscall(serverSocket=socket(AF_UNIX,SOCK_STREAM,0));        //creo la socket del client
                    while(connect(serverSocket,(struct sockaddr*)&sa,sizeof(sa)) == -1){       //tento di connettermi con il server
                        if(errno == ENOENT){                                                   //ancora non esiste la socket del server
                            sleep(1);                                                          //attendi
                        }else {exit(EXIT_FAILURE);}
                    }
                    checkSyscall(write(serverSocket, message, 1+sizeof(unsigned int)+messagelength));        //scrivo l'intero messaggio
                    //fa una write unica che contiente tutto
                    char* data=(char*)malloc(BUFFSIZE);
                    unsigned int length;
                    int nread;
                    while(nread=read(serverSocket,data,1)){       //leggo il primo byte della risposta del server
                        checkSyscall(nread);
                        switch (*data){
                            case MSG_NO:                    //type="N"
                                checkSyscall(read(serverSocket,&length,sizeof(unsigned int)));
                                length=1;
                                printf("\n%s: MSG_NO\n",sockname);      //se name_bib=Alessandria -----> "Alessandria: MSG_NO"
                                break;
                            case MSG_ERROR:                 //type="E"
                                checkSyscall(read(serverSocket,&length,sizeof(unsigned int)));        //leggo la lunghezza del messaggio
                                data=(char*)realloc(data,length);                       //rialloco il buffer data per fittare data
                                checkSyscall(read(serverSocket,data,length));                         //leggo data
                                printf("\n%s: MSG_ERROR\n",sockname);                  
                                printf("%s\n",data);                                    //stampo il messaggio di errore ricevuto
                                break;
                            case MSG_RECORD:                //type="R"
                                checkSyscall(read(serverSocket,&length,sizeof(unsigned int)));        //leggo la lunghezza del messaggio
                                data=(char*)realloc(data,length);           
                                checkSyscall(read(serverSocket,data,length));                         //leggo il record 
                                printf("\n%s: MSG_RECORD\n",sockname);
                                printf("%s\n",data);                                    //stampo il record
                                break;
                            default:
                            break;
                        }
                        memset(data,0,length);                                          //pulisco il buffer data
                    }
                    free(data);                                                         //dealloco data
                    close(serverSocket);                                                //chiudo la connessione con il server
                } 
            }
        }
        free(riga);
        free(message);
        if(emptyConf){                      //se il file bib.conf è vuoto
            printf("bib.conf è vuoto\n");
        }
    fclose(fconf);                          //chiudo il file bib.conf
    freeBook(bookQuery);                    //dealloco il libro
    }else{
        checkFerror(fconf);
    }
    exit(EXIT_SUCCESS);
}