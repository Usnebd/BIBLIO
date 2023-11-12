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
#define UNIX_PATH_MAX 80
#define SIZE 50

int main(int argc, char* argv[]){
    FILE* fconf=fopen("bib.conf","r");
    if(fconf){
        char type;
        char* data;
        unsigned int length;
        size_t size=SIZE;
        char* sockname=(char*)malloc(SIZE);
        struct sockaddr_un sa;
        sa.sun_family=AF_UNIX;
        while(!feof(fconf)){
            getline(&sockname,&size,fconf);
            strncpy(sa.sun_path,sockname ,UNIX_PATH_MAX);
            int welcomeSocket=socket(AF_UNIX,SOCK_STREAM,0);
            while(connect(welcomeSocket,(struct sockaddr*)&sa,sizeof(sa)) == -1){
                if(errno == ENOENT){
                    sleep(1);
                }else {exit(EXIT_FAILURE);}
            }
            length=0;
            Book_t* bookQuery;
            bool wrongInput=false;
            type='Q';
            for(int i=1;i<=argc;i++){
                if(strncmp(argv[i],"--autore=",9)==0){
                    if((bookQuery->autore)==NULL){
                        strcpy(bookQuery->autore->val,argv[i]+9);
                        length=length+strlen("autore: ")+strlen(argv[i]+9)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--titolo=",9)==0){
                    if((bookQuery->titolo)==NULL){
                        strcpy(bookQuery->titolo,argv[i]+9);
                        length=length+strlen("titolo: ")+strlen(argv[i]+9)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--editore=",10)==0){
                    if((bookQuery->editore)==NULL){
                        strcpy(bookQuery->editore,argv[i]+10);
                        length=length+strlen("editore: ")+strlen(argv[i]+10)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--nota=",7)==0){
                    if((bookQuery->nota)==NULL){
                        strcpy(bookQuery->nota,argv[i]+7);
                        length=length+strlen("nota: ")+strlen(argv[i]+7)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--collocazione=",15)==0){
                    if((bookQuery->collocazione)==NULL){
                        strcpy(bookQuery->collocazione,argv[i]+15);
                        length=length+strlen("collocazione: ")+strlen(argv[i]+15)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--luogo_pubblicazione=",22)==0){
                    if((bookQuery->luogo_pubblicazione)==NULL){
                        strcpy(bookQuery->luogo_pubblicazione,argv[i]+22);
                        length=length+strlen("luogo_pubblicazione: ")+strlen(argv[i]+22)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--anno=",7)==0){
                    if((bookQuery->anno)!=0){
                        bookQuery->anno=atoi(argv[i]+7);
                        length=length+strlen("anno: ")+strlen(argv[i]+7)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strncmp(argv[i], "--descrizione_fisica=",21)==0){
                    if((bookQuery->descrizione_fisica)==NULL){
                        strcpy(bookQuery->descrizione_fisica,argv[i]+21);
                        length=length+strlen("descrizione_fisica: ")+strlen(argv[i]+21)+strlen(";");
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
                close(welcomeSocket);
                _exit(EXIT_FAILURE);
            }
            data=(char*)malloc(length);
            bookToRecord(bookQuery,data,'N');
            // Scrive il tipo della struct
            write(welcomeSocket, &type, sizeof(type));
            // Scrive la lunghezza della struct
            write(welcomeSocket, &length, sizeof(length));
            // Scrive i dati della struct
            write(welcomeSocket, data, sizeof(*data));
            char* buff=(char*)malloc(sizeof(unsigned int));
            while((read(welcomeSocket,buff,1))!=0){
                switch (buff[0]){
                    case MSG_NO:
                        read(welcomeSocket,&length,sizeof(unsigned int));
                        printf("MSG_NO: data length=%d\n",length);
                        printf("Closed connection...\n");
                        free(buff);
                        close(welcomeSocket);
                        free(sockname);
                        _exit(EXIT_FAILURE);
                        break;
                    case MSG_ERROR:
                        read(welcomeSocket,&length,sizeof(unsigned int));
                        free(buff);
                        buff=(char*)malloc(length);
                        read(welcomeSocket,buff,length);
                        printf("%s\n",buff);
                        free(buff);
                        printf("Closed connection...\n");
                        close(welcomeSocket);
                        free(sockname);
                        _exit(EXIT_FAILURE);
                        break;
                    case MSG_RECORD:
                        read(welcomeSocket,&length,sizeof(unsigned int));
                        free(buff);
                        buff=(char*)malloc(length);
                        read(welcomeSocket,buff,length);
                        printf("%s\n",buff);
                        free(buff);
                        break;
                    default:
                        printf("Closed connection...\n");
                        free(buff);
                        close(welcomeSocket);
                        free(sockname);
                        _exit(EXIT_FAILURE);
                    break;
                }
            }
        }
    }else{
        perror("Errore apertura file\n");
        _exit(EXIT_FAILURE);
    }
    _exit(EXIT_SUCCESS);
}