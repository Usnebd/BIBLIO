#include"bibserver.h"
#define SIZE 1024
#define UNIX_PATH_MAX 80
#define N 100

int main(int argc, char* argv[]){
    if(argc!=4){
        printf("Not the right amount of args\n");
        _exit(EXIT_FAILURE);
    }
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int n_workers = atoi(argv[3]);
    size_t size=SIZE;
    char* riga=(char*)malloc(SIZE);
    //aggiorno il file bib.conf aggiungendo il socket del server attuale
    checkFromConf(name_bib);
    //fine aggiornamento
    int nchar_readed;
    FILE* fin = fopen(file_name,"r");
    Elem* head=NULL;
    bool firstIteration=true;
    if(fin){
        while((nchar_readed=getline(&riga,&size,fin)) && !feof(fin)){
            if(nchar_readed!=1)
            {   
                char temp_riga[strlen(riga)];
                strcpy(temp_riga,riga);
                Book_t* book=(Book_t*)malloc(sizeof(Book_t));
                recordToBook(riga,book);
                Elem* elem=(Elem*)malloc(sizeof(Elem));
                elem->val=book;
                if(firstIteration){
                    head=elem;
                    firstIteration=false;
                }else{
                    elem->next=head;
                    head=elem;
                }
            }
        }
        char path[strlen(name_bib)+1];
        strcpy(path,"./");
        strcat(path,name_bib);
        FILE* flog=fopen(strcat(name_bib,".log"),"w");
        //aggiungere if(flog)
        struct sockaddr_un sa_server;
        unlink(path);
        strncpy(sa_server.sun_path,path,UNIX_PATH_MAX);
        sa_server.sun_family=AF_UNIX;
        int welcomeSocket=socket(AF_UNIX,SOCK_STREAM,0);
        bind(welcomeSocket,(struct sockaddr*)&sa_server,sizeof(sa_server));
        listen(welcomeSocket,SOMAXCONN);
    
        fd_set set, rdset;
        FD_ZERO(&set);
        FD_SET(welcomeSocket,&set);
        int fd_num=0;
        if (welcomeSocket > fd_num) fd_num = welcomeSocket;
        pthread_mutex_t m;
        pthread_mutex_init(&m,NULL);
        Queue_t* q= initQueue();
        arg_t threadArgs;
        threadArgs.q=q;
        threadArgs.mutex=&m;
        threadArgs.fd_num=&fd_num;
        threadArgs.list=head;
        threadArgs.clients=&set;
        for(int j=0;j<n_workers;j++){
            pthread_t tid;
            pthread_create(&tid,NULL,worker,&threadArgs);
            pthread_detach(tid);
        }
        while(1){
            pthread_mutex_lock(&m);
            rdset=set;
		    int currMax=fd_num;
            pthread_mutex_unlock(&m);
            select(currMax+1,&rdset,NULL,NULL,NULL);
            for (int i=0;i<currMax+1;i++){
                if (FD_ISSET(i,&rdset)){
                    if (i==welcomeSocket){
                        int fd_c=accept(welcomeSocket, NULL,0);
                        printf("E' arrivato un cliente \n");
                        pthread_mutex_lock(&m);
                        FD_SET(fd_c,&set);
                        if(fd_c>fd_num)
                            fd_num=fd_c;
                        pthread_mutex_unlock(&m);
                    }else{
                        int* client=malloc(sizeof(int));
                        *client=i;
                        push(q,client);
                        pthread_mutex_lock(&m);
                        FD_CLR(i,&set);
                        pthread_mutex_unlock(&m);
                    }
                }
            }
	    }
        if(ferror(flog)){
            perror("Errore durante la lettura del file di log\n");
        }
        fclose(flog);
        fclose(fin);
    }
    else if(ferror(fin)){
        perror("Errore durante la lettura del file contentente i record\n");
    }  
    deleteFromConf(name_bib); //cancello name_bib da bib.conf perché il server non è più operativo
    _exit(EXIT_SUCCESS);
}

void* worker(void* args){
    Queue_t* q=((arg_t*)args)->q;
	int* fd_num=(((arg_t*)args)->fd_num);
	fd_set* clients=((arg_t*)args)->clients;
    Elem* node=((arg_t*)args)->list;
	pthread_mutex_t* m=((arg_t*)args)->mutex;
	while(1){
		int *fd=(int*)pop(q);
        unsigned int length;
        memset(&length,0,sizeof(length));
        char type;
		read(*fd,&type,1);
        read(*fd,&length,sizeof(unsigned int));
        char* buff=(char*)malloc(length);
        memset(buff,0,sizeof(buff));
        read(*fd,buff,length);
        Book_t* book=(Book_t*)malloc(sizeof(Book_t)); 
        memset(book, 0, sizeof(Book_t));
        recordToBook(buff,book);
        bool noMatches=true;
        switch(type){
            case MSG_QUERY:
                noMatches=true;
                while(node!=NULL){
                    if(matchElemBook(book,node->val)){
                        buff=(char*)realloc(buff,SIZE);
                        memset(buff,0,SIZE);
                        bookToRecord(node->val,buff);
                        noMatches=false;
                        write(*fd,"R",1);
                        unsigned int dataLength=strlen(buff)+1;
                        write(*fd,&dataLength,sizeof(unsigned int)); 
                        write(*fd,buff,dataLength);
                    }
                    if(node->next!=NULL){
                        if(node->next->val!=NULL){
                            node=node->next;
                        }
                    }else{
                        node=NULL;
                    }
                }
                break;
            case MSG_LOAN:
                noMatches=true;
                while(node!=NULL){
                    if(matchElemBook(book,node->val)){
                        struct tm loanTime;
                        bool available=false;
                        time_t timestamp = time(NULL);
                        struct tm* currentTime = localtime(&timestamp);
                        if(strcmp(node->val->prestito,"")!=0){
                            char date[strlen(node->val->prestito)];
                            strcpy(date,node->val->prestito);
                            char* tokDMY=strtok(date," ");  //DMY = Day Month Year
                            char* tokSMH=strtok(NULL," ");      //SMH = Seconds Minutes Hours
                            
                            loanTime.tm_mday=atoi(strtok(tokDMY,"-"));
                            loanTime.tm_mon=atoi(strtok(NULL,"-")-1);
                            loanTime.tm_year=atoi(strtok(NULL,"-"))-1900;

                            loanTime.tm_hour=atoi(strtok(tokSMH,":"));
                            loanTime.tm_min=atoi(strtok(NULL,":"));
                            loanTime.tm_sec=atoi(strtok(NULL,":"));
                            
                            if(difftime(mktime(currentTime),mktime(&loanTime))>0){  //data di scadenza (loanTime) è passata
                                available=true;
                            }
                        }else{
                            available=true;
                        }
                        if(available){
                            noMatches=false;
                            memset(node->val->prestito, 0, sizeof(node->val->prestito));

                            int offset = 0;  // Inizializza l'offset a 0

                            if(currentTime->tm_mday<10){
                                offset += sprintf(node->val->prestito + offset, "%d", 0);
                            }
                            offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_mday);
                            offset += sprintf(node->val->prestito + offset, "-");
                            if(currentTime->tm_mon<10){
                                offset += sprintf(node->val->prestito + offset, "%d", 0);
                            }
                            offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_mon + 1);
                            offset += sprintf(node->val->prestito + offset, "-");
                            offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_year + 1900);
                            offset += sprintf(node->val->prestito + offset, " ");
                            if(currentTime->tm_hour<10){
                                offset += sprintf(node->val->prestito + offset, "%d", 0);
                            }
                            offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_hour);
                            offset += sprintf(node->val->prestito + offset, ":");
                            if(currentTime->tm_min<10){
                                offset += sprintf(node->val->prestito + offset, "%d", 0);
                            }
                            offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_min);
                            offset += sprintf(node->val->prestito + offset, ":");
                            if(currentTime->tm_sec<10){
                                offset += sprintf(node->val->prestito + offset, "%d", 0);
                            }
                            offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_sec);

                            buff=(char*)realloc(buff,SIZE);
                            memset(buff,0,SIZE);
                            unsigned int dataLength=bookToRecord(node->val,buff);
                            write(*fd,"L",1);
                            write(*fd,&dataLength,sizeof(unsigned int)); 
                            write(*fd,buff,dataLength);
                        }                        
                    }
                    if(node->next!=NULL){
                        if(node->next->val!=NULL){
                            node=node->next;
                        }
                    }else{
                        node=NULL;
                    }
                }
                break;
            default:
                break;
        }
        if(noMatches){
            write(*fd,"N",1);
            write(*fd,0,sizeof(0));
        }
		close(*fd);
        freeBook(book);
        free(buff);
	}
	return NULL;
}

Book_t* recordToBook(char* riga, Book_t* book){
    int n_attributes=countAttributes(riga);
    char* field[n_attributes];
    char* str=strtok(riga,";");
    field[0]=(char*)malloc(strlen(str));
    strcpy(field[0],str);
    for(int i=1;i<n_attributes;i++){
        str = strtok(NULL,";");
        field[i]=(char*)malloc(strlen(str));
        strcpy(field[i],str);
    }
    int n_autori=0;
    NodoAutore* previousAuthor;
    bool firstIteration=true;
    int i=0;
    while(i<n_attributes){
        int key_length = strchr(field[i],':') - (field[i] + strspn(field[i], " "));
        char key[key_length + 1];
        strcpy(key,"");
        strncpy(key, field[i]+strspn(field[i], " "), key_length);
        key[key_length] = '\0';
        char* value=strchr(field[i],':')+1+strspn(strchr(field[i],':')+1, " ");
        if(strcmp(key,"autore") == 0){
            NodoAutore* currentAuthor=(NodoAutore*)malloc(sizeof(NodoAutore));
            memset(currentAuthor, 0, sizeof(NodoAutore));
            currentAuthor->val=(char*)malloc(strlen(value));
            strcpy(currentAuthor->val,value);
            if(firstIteration){
                previousAuthor=currentAuthor;
                book->autore=currentAuthor;
                firstIteration=false;
            }else{
                previousAuthor->next=currentAuthor;
                previousAuthor=currentAuthor;
            }
            n_autori++;
        }else if(strcmp(key,"titolo") == 0){
            book->titolo=(char*)malloc(strlen(value)+1);
            strcpy(book->titolo,value);
        }else if(strcmp(key,"editore") == 0){
            book->editore=(char*)malloc(strlen(value)+1);
            strcpy(book->editore,value);
        }else if(strcmp(key,"nota") == 0){
            book->nota=(char*)malloc(strlen(value)+1);
            strcpy(book->nota,value);
        }else if(strcmp(key,"collocazione") == 0){
            book->collocazione=(char*)malloc(strlen(value)+1);
            strcpy(book->collocazione,value);
        }else if(strcmp(key,"luogo_pubblicazione") == 0){
            book->luogo_pubblicazione=(char*)malloc(strlen(value)+1);
            strcpy(book->luogo_pubblicazione,value);
        }else if(strcmp(key,"anno") == 0){
            book->anno=atoi(value);
        }else if(strcmp(key,"prestito") == 0){
            strcpy(book->prestito,value);
        }else if(strcmp(key,"descrizione_fisica") == 0){
            book->descrizione_fisica=(char*)malloc(strlen(value)+1);
            strcpy(book->descrizione_fisica,value);
        }
        free(field[i]);
        i++;
    }
    return book;
}

bool matchElemBook(Book_t* book, Book_t* bookNode){
    bool match=false;
    if(book->autore!=NULL){
        NodoAutore* currAuthor=bookNode->autore;
        while(currAuthor != NULL && currAuthor->val != NULL){
            if(strcmp(currAuthor->val,book->autore->val)==0){
                match=true;
            }
            currAuthor=currAuthor->next;
        }
    }
    if(book->titolo!=NULL){
        if(strcmp(bookNode->titolo,book->titolo)==0){
            match=true;
        }          
    }
    if(book->editore!=NULL){
        if(strcmp(bookNode->editore,book->editore)==0){
            match=true;
        }          
    }
    if(book->anno!=0){
        if(book->anno==bookNode->anno){
            match=true;
        }          
    }
    if(book->nota!=NULL){
        if(strcmp(bookNode->nota,book->nota)==0){
            match=true;
        }          
    }
    if(book->collocazione!=NULL){
        if(strcmp(bookNode->collocazione,book->collocazione)==0){
            match=true;
        }          
    }
    if(book->luogo_pubblicazione!=NULL){
        if(strcmp(bookNode->luogo_pubblicazione,book->luogo_pubblicazione)==0){
            match=true;
        }          
    }
    if(book->descrizione_fisica!=NULL){
        if(strcmp(bookNode->descrizione_fisica,book->descrizione_fisica)==0){
            match=true;
        }          
    }
    return match;
}

bool checkAutore(Book_t* bookNode){
    // Controlla il puntatore `bookNode->autore`.
    return bookNode->autore != NULL;
}

void checkFromConf(char* name_bib){
    FILE* fconf=fopen("bib.conf","a");
    size_t size=N;
    char* riga=(char*)malloc(N);
    if(fconf==NULL){
        perror("errore apertura file bib.conf\n");
        _exit(EXIT_FAILURE);
    }else{
        while(getline(&riga,&size,fconf)&& riga[0]!='\0'){
            if(strcmp(riga,name_bib)==0){             
                perror("nome biblioteca già preso\n");
                _exit(EXIT_FAILURE);
            }
        }
        fprintf(fconf,"%s\n",name_bib);
    }
    fclose(fconf);
    free(riga);
}


void deleteFromConf(char* name_bib){
    FILE* fconf=fopen("bib.conf","a");
    char* buffer=(char*)malloc(N);
    if(fconf==NULL){
        perror("errore apertura file bib.conf\n");
        _exit(EXIT_FAILURE);
    }else{
        int pos = strstr(buffer, name_bib) - buffer;
        // Se la parola è stata trovata
        if (pos != -1) {
            // Rimuovi la parola dalla stringa
            buffer[pos] = '\0';

            // Scrivi il nuovo contenuto del file
            fseek(fconf, 0, SEEK_SET);
            fputs(buffer, fconf);
        }
    }
    fclose(fconf);
    free(buffer);
}

int countAttributes(char* riga){
    int count=0;
    for(int i=0;i<strlen(riga);i++){
        if(riga[i]==';'){
            count++;
        }
    }
    return count;
}