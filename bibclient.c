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
#define SIZE 50
#define BUFFSIZE 300 

int main(int argc, char* argv[]){
    FILE* fconf=fopen("bib.conf","r");
    if(fconf){
        char type;
        size_t size=SIZE;
        char* sockname=(char*)malloc(SIZE);
        struct sockaddr_un sa;
        sa.sun_family=AF_UNIX;
        bool emptyConf=true;
        while(!feof(fconf) && (getline(&sockname,&size,fconf)>1)){
            emptyConf=false;
            char path[1+strlen(sockname)];
            strcpy(path,"./");
            strcat(path,strtok(sockname,"\n"));
            strcpy(sa.sun_path,path);
            int serverSocket=socket(AF_UNIX,SOCK_STREAM,0);
            while(connect(serverSocket,(struct sockaddr*)&sa,sizeof(sa)) == -1){
                if(errno == ENOENT){
                    sleep(1);
                }else {exit(EXIT_FAILURE);}
            }
            Book_t* bookQuery=(Book_t*)malloc(sizeof(Book_t));
            bool wrongInput=false;
            type='Q';
            int i;
            for(i=1;i<argc;i++){
                if(strncmp(argv[i],"--autore=",9)==0){
                    if(bookQuery->autore==NULL){
                        bookQuery->autore=(NodoAutore*)malloc(sizeof(NodoAutore));
                        if(bookQuery->autore->val==NULL){
                            bookQuery->autore->val=(char*)malloc(strlen(argv[i]+9));
                            strcpy(bookQuery->autore->val,argv[i]+9);
                        }else{
                            wrongInput=true;
                        }
                    }
                }else if(strncmp(argv[i], "--titolo=",9)==0){
                    if(bookQuery->titolo==NULL){
                        bookQuery->titolo=(char*)malloc(strlen(argv[i]+9));
                        strcpy(bookQuery->titolo,argv[i]+9);
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--editore=",10)==0){
                    if(bookQuery->editore==NULL){
                        bookQuery->editore=(char*)malloc(strlen(argv[i]+10));
                        strcpy(bookQuery->editore,argv[i]+10);
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--nota=",7)==0){
                    if(bookQuery->nota==NULL){
                        bookQuery->nota=(char*)malloc(strlen(argv[i]+7));
                        strcpy(bookQuery->nota,argv[i]+7);
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--collocazione=",15)==0){
                    if(bookQuery->collocazione==NULL){
                        bookQuery->collocazione=(char*)malloc(strlen(argv[i]+15));
                        strcpy(bookQuery->collocazione,argv[i]+15);
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--luogo_pubblicazione=",22)==0){
                    if(bookQuery->luogo_pubblicazione==NULL){
                        bookQuery->luogo_pubblicazione=(char*)malloc(strlen(argv[i]+22));
                        strcpy(bookQuery->luogo_pubblicazione,argv[i]+22);
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--anno=",7)==0){
                    if(bookQuery->anno!=0){
                        bookQuery->anno=atoi(argv[i]+7);
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--descrizione_fisica=",21)==0){
                    if(bookQuery->descrizione_fisica==NULL){
                        bookQuery->descrizione_fisica=(char*)malloc(strlen(argv[i]+21));
                        strcpy(bookQuery->descrizione_fisica,argv[i]+21);
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "-p")==0){
                    if(type!='L'){
                        type='L';
                    }else{
                        wrongInput=true;
                    }
                }else{
                    wrongInput=true;
                }
            }
            if(wrongInput){
                printf("Wrong format of args\n");
                close(serverSocket);
                _exit(EXIT_FAILURE);
            }

            char buffer[BUFFSIZE];
            strcpy(buffer,"");
            unsigned int length=bookToRecord(bookQuery,buffer,'N');
            freeBook(bookQuery);
            // Copia il tipo della struct nel buffer
            char* data=(char*)malloc(1+length+sizeof(unsigned int));
            data[0] = type;
            length++; //perché la stringa data conterrà anche il carattere di fine stringa /0
            // Copia la lunghezza della struct nel buffer
            memcpy(&data[1], &length, sizeof(unsigned int));
            // Copia i dati della struct nel buffer
            memcpy(&data[5], buffer, length);
            write(serverSocket, data, 1+sizeof(unsigned int)+length);
            //fa una write unica che contiente tutto
            data=(char*)realloc(data,1);
            while((read(serverSocket,data,1))!=0){
                switch (*data){
                    case MSG_NO:
                        read(serverSocket,&length,sizeof(unsigned int));
                        printf("MSG_NO\n");
                        printf("Closed connection...\n");
                        free(data);
                        close(serverSocket);
                        free(sockname);
                        _exit(EXIT_FAILURE);
                        break;
                    case MSG_ERROR:
                        read(serverSocket,&length,sizeof(unsigned int));
                        data=(char*)realloc(data,length);
                        read(serverSocket,data,length);
                        printf("MSG_ERROR\n%s",data);
                        free(data);
                        printf("Closed connection...\n");
                        close(serverSocket);
                        free(sockname);
                        _exit(EXIT_FAILURE);
                        break;
                    case MSG_RECORD:
                        read(serverSocket,&length,sizeof(unsigned int));
                        data=(char*)realloc(data,length);
                        read(serverSocket,data,length);
                        printf("MSG_RECORD\n%s",data);
                        free(data);
                        break;
                    default:
                        printf("Closed connection...\n");
                        free(data);
                        close(serverSocket);
                        free(sockname);
                        _exit(EXIT_FAILURE);
                    break;
                }
            }
        }
        if(emptyConf==true){
            printf("bib.conf è vuoto\n");
        }
        free(sockname);
    }else{
        perror("Errore apertura file\n");
        _exit(EXIT_FAILURE);
    }
    _exit(EXIT_SUCCESS);
}