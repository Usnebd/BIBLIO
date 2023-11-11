#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#include "structures.h"
#define UNIX_PATH_MAX 80
#define SIZE 50

int main(int argc, char* argv[]){
    FILE* fconf=fopen("bib.conf","r");
    if(fconf){
        char* buff=(char*)malloc(N);
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
            int socket=socket(AF_UNIX,SOCK_STREAM,0);
            while(connect(socket,(struct sockaddr*)&sa),sizeof(sa) == -1){
                if(errno == ENOENT){
                    sleep(1);
                }else {exit(EXIT_FAILURE);}
            }
            length=0;
            Book_t* bookQuery;
            bool wrongInput=false;
            type="Q";
            for(int i=1;i<=argc;i++){
                if(strcmp(argv[i],"--autore=",9)==0){
                    if((bookQuery->autore)==NULL){
                        strcpy(bookQuery->autore[0],argv[i]+9);
                        length=length+strlen("autore: ")+strlen(argv[i]+9)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--titolo=",9)==0){
                    if((bookQuery->titolo)==NULL){
                        strcpy(bookQuery->titolo,argv[i]+9);
                        length=length+strlen("titolo: ")+strlen(argv[i]+9)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--editore=",10)==0){
                    if((bookQuery->editore)==NULL){
                        strcpy(bookQuery->editore,argv[i]+10);
                        length=length+strlen("editore: ")+strlen(argv[i]+10)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--nota=",7)==0){
                    if((bookQuery->nota)==NULL){
                        strcpy(bookQuery->nota,argv[i]+7);
                        length=length+strlen("nota: ")+strlen(argv[i]+7)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--collocazione=",15)==0){
                    if((bookQuery->collocazione)==NULL){
                        strcpy(bookQuery->collocazione,argv[i]+15);
                        length=length+strlen("collocazione: ")+strlen(argv[i]+15)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--luogo_pubblicazione=",22)==0){
                    if((bookQuery->luogo_pubblicazione)==NULL){
                        strcpy(bookQuery->luogo_pubblicazione,argv[i]+22);
                        length=length+strlen("luogo_pubblicazione: ")+strlen(argv[i]+22)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--anno=",7)==0){
                    if((bookQuery->anno)==NULL){
                        strcpy(bookQuery->anno,argv[i]+7);
                        length=length+strlen("anno: ")+strlen(argv[i]+7)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "--descrizione_fisica=",21)==0){
                    if((bookQuery->descrizione_fisica)==NULL){
                        strcpy(bookQuery->descrizione_fisica,argv[i]+21);
                        length=length+strlen("descrizione_fisica: ")+strlen(argv[i]+21)+strlen(";");
                    }else{
                        wrongInput=true;
                    }
                }else if(strcmp(argv[i], "-p")==0){
                    if(strcmp(type,"L")!=0){
                        type="L";
                    }else{
                        wrongInput=true;
                    }
                }else{
                    wrongInput=true;
                }
            }
            if(wrongInput){
                printf("Wrong format of args\n");
                close(socket);
                _exit(EXIT_FAILURE);
            }
            data=(char*)malloc(length);
            if(bookQuery->autore[0]!=NULL){
                strcat(data,"autore: ");
                strcat(data,bookQuery->autore[0]);
                strcat(data,";");
            }
            if((bookQuery->titolo)!=NULL){
                strcat(data,"titolo: ");
                strcat(data,bookQuery->titolo);
                strcat(data,";");
            }
            if((bookQuery->editore)!=NULL){
                strcat(data,"editore: ");
                strcat(data,bookQuery->editore);
                strcat(data,";");
            }
            if((bookQuery->anno)!=NULL){
                strcat(data,"anno: ");
                strcat(data,bookQuery->anno);
                strcat(data,";");
            }
            if((bookQuery->nota)!=NULL){
                strcat(data,"nota: ");
                strcat(data,bookQuery->nota);
                strcat(data,";");
            }
            if((bookQuery->collocazione)!=NULL){
                strcat(data,"collocazione: ");
                strcat(data,bookQuery->collocazione);
                strcat(data,";");
            }
            if((bookQuery->luogo_pubblicazione)!=NULL){
                strcat(data,"luogo_pubblicazione: ");
                strcat(data,bookQuery->luogo_pubblicazione);
                strcat(data,";");
            }
            if((bookQuery->descrizione_fisica)!=NULL){
                strcat(data,"descrizione_fisica: ");
                strcat(data,bookQuery->descrizione_fisica);
                strcat(data,";");
            }
            // Scrive il tipo della struct
            write(sockname, &type, sizeof(type));
            // Scrive la lunghezza della struct
            write(sockname, &length, sizeof(length));
            // Scrive i dati della struct
            write(sockname, data, sizeof(*data));
            while((read(sockname,buff,N))!=0){
                switch (buff){
                    case 'N':
                        printf("0 result\n");
                        shutClient(sockname,false);
                        break;
                    case 'E':
                        read(sockname,buff,N);
                        length=atoi(buff);
                        if(length<N){
                            read(sockname,buff,length);
                        }else{
                            free(buff);
                            buff=(char*)malloc(length);
                            read(sockname,buff,length);
                        }
                        printf("%s\n",buff);
                        break;
                    case 'R':
                        read(sockname,buff,N);
                        length=atoi(buff);
                        if(length<N){
                            read(sockname,buff,length);
                        }else{
                            free(buff);
                            buff=(char*)malloc(length);
                            read(sockname,buff,length);
                        }
                        printf("%s\n",buff);
                        break;
                    default:
                        shutClient(sockname,false);
                    break;
                }
            }
            shutClient(sockname,true);
        }
    }else{
        perror("Errore apertura file");
    }
    printf("Closed connection...\n");
    close(fd);
    free(fd);
    _exit(EXIT_SUCCESS);
}

