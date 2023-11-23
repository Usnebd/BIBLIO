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
    FILE* fconf=fopen("bib.conf","r");
    if(fconf){
        Book_t* bookQuery=(Book_t*)malloc(sizeof(Book_t));
        char type='Q';
        for(int i=1;i<argc;i++){
            if(strncmp(argv[i],"--autore=",9)==0){
                bookQuery->autore=(NodoAutore*)malloc(sizeof(NodoAutore));
                bookQuery->autore->val=(char*)malloc(strlen(argv[i]+9));
                strcpy(bookQuery->autore->val,argv[i]+9);
            }else if(strncmp(argv[i], "--titolo=",9)==0){
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
            }else if(strcmp(argv[i], "-p")==0){
                type='L';
            }else{
                printf("Wrong format of args\n");
                exit(EXIT_FAILURE);
            }
        }
        char buffer[BUFFSIZE];
        memset(buffer,0,BUFFSIZE);
        unsigned int messagelength=bookToRecord(bookQuery,buffer)+1;
        // Copia il tipo della struct nel buffer
        char* message=(char*)malloc(BUFFSIZE);
        memcpy(&message[0], &type, 1);
        // Copia la lunghezza della struct nel buffer
        memcpy(&message[1], &messagelength, sizeof(unsigned int));
        // Copia i dati della struct nel buffer
        memcpy(&message[1+sizeof(messagelength)], buffer, messagelength);
        size_t size=SIZE;
        struct sockaddr_un sa;
        sa.sun_family=AF_UNIX;
        bool emptyConf=true;
        char* sockname;
        char* riga=(char*)malloc(SIZE);
        int nchar;
        while((nchar=getline(&riga,&size,fconf))!=-1){
            if(nchar!=0 && nchar!=1){
               emptyConf=false;
                strtok(riga,",");
                strtok(riga,":");
                sockname=strtok(NULL,":");
                if(strspn(sockname, "X") != strlen(sockname)){ //BibName:XXXXX,Sockpath:XXXXX   nome invalido
                    char path[1+strlen(sockname)];             //un server era attivo ma poi è andato offline
                    strcpy(path,"./");
                    strcat(path,strtok(sockname,":"));
                    strcpy(sa.sun_path,path);
                    int serverSocket=socket(AF_UNIX,SOCK_STREAM,0);
                    while(connect(serverSocket,(struct sockaddr*)&sa,sizeof(sa)) == -1){
                        if(errno == ENOENT){
                            sleep(1);
                        }else {exit(EXIT_FAILURE);}
                    }
                    write(serverSocket, message, 1+sizeof(unsigned int)+messagelength);
                    //fa una write unica che contiente tutto
                    char* data=(char*)malloc(BUFFSIZE);
                    unsigned int length;
                    while(read(serverSocket,data,1)){
                        switch (*data){
                            case MSG_NO:
                                read(serverSocket,&length,sizeof(unsigned int));
                                length=1;
                                printf("\n%s: MSG_NO\n",sockname);
                                break;
                            case MSG_ERROR:
                                read(serverSocket,&length,sizeof(unsigned int));
                                data=(char*)realloc(data,length);
                                read(serverSocket,data,length);
                                printf("\n%s: MSG_ERROR\n",sockname);
                                printf("%s\n",data);
                                break;
                            case MSG_RECORD:
                                read(serverSocket,&length,sizeof(unsigned int));
                                data=(char*)realloc(data,length);
                                read(serverSocket,data,length);
                                printf("\n%s: MSG_RECORD\n",sockname);
                                printf("%s\n",data);
                                break;
                            default:
                            break;
                        }
                        memset(data,0,length);
                    }
                    free(data);
                    close(serverSocket);
                } 
            }
        }
        free(riga);
        free(message);
        if(emptyConf){
            perror("bib.conf è vuoto\n");
        }
    fclose(fconf);
    freeBook(bookQuery);
    }else{
        perror("Errore apertura file\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}