#include"bibserver.h"
#define SIZE 1024
#define UNIX_PATH_MAX 80
#define N 100

volatile int sigflag=0;

int main(int argc, char* argv[]){
    if(argc!=4){
        printf("Not the right amount of args\n");
        _exit(EXIT_FAILURE);
    }
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int n_workers = atoi(argv[3]);
    size_t size=SIZE;
    addBibToConf(name_bib);
    //registro gestore
    struct sigaction s;
    memset(&s,0,sizeof(s));
    s.sa_handler=gestore;
    //installo il gestore
    sigaction(SIGINT,&s,NULL);
    sigaction(SIGTERM,&s,NULL);
    FILE* fin = fopen(file_name,"r");
    if(fin){
        Elem* head=NULL;
        bool firstIteration=true;
        int nchar=0;
        char* riga=(char*)malloc(SIZE);
        while((nchar=getline(&riga,&size,fin))!=-1){
            if(nchar!=0 && nchar!=1){
                Book_t* book=(Book_t*)malloc(sizeof(Book_t));
                memset(book, 0, sizeof(Book_t));
                recordToBook(riga,book);
                Elem* elem=(Elem*)malloc(sizeof(Elem));
                elem->val=book;
                if(firstIteration){
                    head=elem;
                    firstIteration=false;
                }else if(isAlreadyPresent(book,head)==false){
                    elem->next=head;
                    head=elem;
                }else{
                    freeBook(book);
                    free(elem);
                }
            }
        }
        free(riga);
        char path[strlen(name_bib)+1];
        char logName[strlen(name_bib)+strlen(".log")];
        sprintf(path,"./%s",name_bib);
        sprintf(logName,"%s.log",name_bib);
        FILE* flog=fopen(logName,"w");
        if(ferror(flog)){
            perror("Errore durante la lettura del file di log\n");
            _exit(EXIT_FAILURE);
        }
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
        Queue_t* q=initQueue();
        pthread_mutex_t m;
        pthread_mutex_init(&m,NULL);
        arg_t threadArgs;
        threadArgs.q=q;
        threadArgs.list=head;
        threadArgs.mutex=&m;
        threadArgs.flog=flog;
        pthread_t threadArray[n_workers];
        for(int j=0;j<n_workers;j++){
            pthread_t tid;
            threadArray[j]=tid;
            pthread_create(&tid,NULL,worker,&threadArgs);

        }
        while(1){
            if(sigflag==-1){
                break;
            }
            rdset=set;
            if(select(fd_num+1,&rdset,NULL,NULL,NULL)!=-1){
                for (int i=0;i<fd_num+1;i++){
                    if (FD_ISSET(i,&rdset)){
                        if (i==welcomeSocket){
                            int fd_c=accept(welcomeSocket, NULL,0);
                            printf("E' arrivato un cliente \n");
                            FD_SET(fd_c,&set);
                            if(fd_c>fd_num)
                                fd_num=fd_c;
                        }else{
                            int* client=malloc(sizeof(int));
                            *client=i;
                            push(q,client);
                            FD_CLR(i,&set);
                            fd_num=aggiornaMax(set,fd_num);
                        }
                    }
                }
            }
	    }
        for(int i=0;i<n_workers;i++){
            int* endWorkerSignal=(int*)malloc(sizeof(int));
            *endWorkerSignal=-1;
            push(q,endWorkerSignal);
        }
        for(int i=0;i<n_workers;++i) {
            pthread_join(threadArray[i], NULL);
        }
        unlink(path);
        fclose(flog);
        fclose(fin);
        deleteFromConf(name_bib);
        dumpRecord(file_name,head);
    }
    else if(ferror(fin)){
        perror("Errore durante la lettura del file contentente i record\n");
    }  
    _exit(EXIT_SUCCESS);
}

void dumpRecord(char* filename, Elem* head){
    FILE* fin=fopen(filename,"w");
    if(fin){
        ftruncate(fileno(fin), 0);
        char* record=(char*)malloc(SIZE);
        int offset;
        NodoAutore* nodeAuthor;
        Book_t* book;
        while(head!=NULL){
            bool firstIteration=true;
            offset=0;
            book=head->val;
            nodeAuthor=book->autore;
            while(nodeAuthor!=NULL){
                if(nodeAuthor->val!=NULL){
                    if(firstIteration){
                        firstIteration=false;
                        offset +=sprintf(record,"autore: %s;",nodeAuthor->val);
                    }else{
                        offset +=sprintf(record+offset," autore: %s;",nodeAuthor->val);
                    }
                }
                nodeAuthor=nodeAuthor->next;
            }
            if(book->titolo!=NULL){
                offset +=sprintf(record+offset," titolo: %s;",book->titolo);
            }
            if(book->editore!=NULL){
                offset +=sprintf(record+offset," editore: %s;",book->editore);
            }
            if(book->anno!=0){
                offset +=sprintf(record+offset," anno: %d;",book->anno);
            }
            if(book->nota!=NULL){
                offset +=sprintf(record+offset," nota: %s;",book->nota);
            }
            if(book->collocazione!=NULL){
                offset +=sprintf(record+offset," collocazione: %s;",book->collocazione);
            }
            if(book->descrizione_fisica!=NULL){
                offset +=sprintf(record+offset," descrizione_fisica: %s;",book->descrizione_fisica);
            }
            if(book->luogo_pubblicazione!=NULL){
                offset +=sprintf(record+offset," luogo_pubblicazione: %s;",book->luogo_pubblicazione);
            }
            if(book->prestito!=NULL){
                offset +=sprintf(record+offset," prestito: %s;",book->prestito);
            }
            strcat(record,"\n");
            offset++;
            fwrite(record+strspn(record," "),1,offset,fin);
            Elem* tmp=head;
            head=head->next;
            freeBook(tmp->val);
            free(tmp);
        }
        free(record);
        fclose(fin);
    }else if(ferror(fin)){
        perror("Errore durante la lettura del file contentente i record\n");
        _exit(EXIT_FAILURE);
    }  
}

void deleteFromConf(char* name_bib){
    FILE* fconf=fopen("bib.conf","r+");
    size_t size=N;
    char* riga=(char*)malloc(N);
    if(fconf==NULL){
        perror("errore apertura file bib.conf\n");
        _exit(EXIT_FAILURE);
    }else{
        int nchar=0;
        long pos_inizio = 0;
        long pos_fine = 0;
        while((nchar=getline(&riga,&size,fconf))!=-1){
            if(nchar != 1 && nchar != 0){
                pos_inizio = pos_fine;
                pos_fine = ftell(fconf);
                if (strstr(riga, name_bib) != NULL) {
                    
                    fseek(fconf, pos_inizio, SEEK_SET);
                    char tmp[strlen(riga)];
                    char tmpName[strlen(name_bib)];
                    memset(tmpName,'X',strlen(name_bib));
                    tmpName[strlen(name_bib)]='\0';
                    sprintf(tmp,"BibName:%s,Sockpath:%s\n",tmpName,tmpName);
                    fwrite(tmp, 1, strlen(riga), fconf);
                    fseek(fconf, 0, SEEK_END);
                }
                memset(riga,0,N);
            }
        }
    }
    fclose(fconf);
    free(riga);
}

void addBibToConf(char* name_bib){
    FILE* fconf=fopen("bib.conf","r+");
    size_t size=N;
    char* riga=(char*)malloc(N);
    if(fconf==NULL){
        perror("errore apertura file bib.conf\n");
        _exit(EXIT_FAILURE);
    }else{
        while(!feof(fconf)){
            if((getline(&riga,&size,fconf))>1){
                strtok(riga,",");
                strtok(riga,":");
                char* tok=strtok(NULL,":");
                if(strcmp(tok,name_bib)==0){             
                    perror("nome biblioteca già preso\n");
                    _exit(EXIT_FAILURE);
                }
                memset(riga,0,N);
            }
        }
        fprintf(fconf,"BibName:%s,Sockpath:%s\n",name_bib,name_bib);
    }
    fclose(fconf);
    free(riga);
}

int aggiornaMax(fd_set set, int max ){
	while(!FD_ISSET(max,&set)) max--;
    return max;
}

bool equalAuthors(Book_t* book, Book_t* bookNode){
    NodoAutore* bookAuthor= book->autore;
    NodoAutore* bookNodeAuthor;
    while(bookAuthor!=NULL){
        bookNodeAuthor=bookNode->autore;
        bool authorPresent=false;
        while(bookNodeAuthor!=NULL){
            if(strcmp(bookNode->autore->val, book->autore->val)==0){
                authorPresent=true;
            }
            bookNodeAuthor=bookNodeAuthor->next;
        }
        if(authorPresent==false){
            return false;
        }
        bookAuthor=bookAuthor->next;
    }
    return true;
}

int nullFieldsCount(Book_t* book){
    int i=0;
    if(book->titolo==NULL){
        i++;
    }
    if(book->anno==0){
        i++;
    }
    if(book->collocazione==NULL){
        i++;
    }
    if(book->descrizione_fisica==NULL){
        i++;
    }
    if(book->editore==NULL){
        i++;
    }
    if(book->luogo_pubblicazione==NULL){
        i++;
    }
    if(book->prestito==NULL){
        i++;
    }
    if(book->nota==NULL){
        i++;
    }
    return i;
}

bool isAlreadyPresent(Book_t* book, Elem* head){
    Elem* currElem=head;
    while(currElem!=NULL){
        bool fieldEqual=true; 
        Book_t* bookNode=currElem->val;   
        if(nullFieldsCount(book)!=nullFieldsCount(bookNode)){
            fieldEqual=false;
        }else{
            if(book->titolo!=NULL && bookNode->titolo!=NULL){
                if(strcmp(book->titolo,bookNode->titolo)!=0){
                    fieldEqual=false;
                }
            }
            if(book->anno!=0 && bookNode->anno!=0){
                if(book->anno!=bookNode->anno){
                    fieldEqual=false;
                }
            }
            if(book->nota!=NULL && bookNode->nota!=NULL){
                if(strcmp(book->nota,bookNode->nota)!=0){
                    fieldEqual=false;
                }
            }
            if(book->collocazione!=NULL && bookNode->collocazione!=NULL){
                if(strcmp(book->collocazione,bookNode->collocazione)!=0){
                    fieldEqual=false;
                }
            }
            if(book->editore!=NULL && bookNode->editore!=NULL){
                if(strcmp(book->editore,bookNode->editore)!=0){
                    fieldEqual=false;
                }
            }
            if(book->descrizione_fisica!=NULL && bookNode->descrizione_fisica!=NULL){
                if(strcmp(book->descrizione_fisica,bookNode->descrizione_fisica)!=0){
                    fieldEqual=false;
                }
            }
            if(book->luogo_pubblicazione!=NULL && bookNode->luogo_pubblicazione!=NULL){
                if(strcmp(book->luogo_pubblicazione,bookNode->luogo_pubblicazione)!=0){
                    fieldEqual=false;
                }
            }
            if(book->prestito!=NULL && bookNode->prestito!=NULL){
                if(strcmp(book->prestito,bookNode->prestito)!=0){
                    fieldEqual=false;
                }
            }
        }
        if(fieldEqual){ //sono uguali fino a prestito, non rimane che verificare autore
            if(equalAuthors(book,bookNode)){
                return true;
            }
        }else{
            currElem=currElem->next;
        }
    }
    return false;
}

void* worker(void* args){
    Queue_t* q=((arg_t*)args)->q;
    Elem* node=((arg_t*)args)->list;
    pthread_mutex_t* m=((arg_t*)args)->mutex;
    FILE* flog=((arg_t*)args)->flog;
    int fd=0;
	while(sigflag!=-1){
        int* ftemp=(int*)pop(q);
        fd=*ftemp;
        free(ftemp);
        if(fd!=-1){
            unsigned int length;
            memset(&length,0,sizeof(length));
            char type;
            read(fd,&type,1);
            read(fd,&length,sizeof(unsigned int));
            char* buff = (char*)malloc(length + 1);
            memset(buff, 0, length + 1);  // Inizializza la memoria allocata a zero
            read(fd,buff,length);
            Book_t* book=(Book_t*)malloc(sizeof(Book_t)); 
            memset(book, 0, sizeof(Book_t));
            recordToBook(buff,book);
            bool noMatches=true;
            int queryCount=0;
            switch(type){
                case MSG_QUERY:
                    noMatches=true;
                    while(node!=NULL){
                        if(matchElemBook(book,node->val)){
                            buff=(char*)realloc(buff,SIZE);
                            memset(buff,0,SIZE);
                            bookToRecord(node->val,buff);
                            noMatches=false;
                            write(fd,"R",1);
                            unsigned int dataLength=strlen(buff);
                            write(fd,&dataLength,sizeof(unsigned int)); 
                            write(fd,buff,dataLength);
                            strcat(buff,"\n");
                            pthread_mutex_lock(m);
                            fwrite(buff,1,strlen(buff),flog);
                            pthread_mutex_unlock(m);
                            queryCount++;
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
                                write(fd,"R",1);
                                write(fd,&dataLength,sizeof(unsigned int)); 
                                write(fd,buff,dataLength);
                                strcat(buff,"\n");
                                pthread_mutex_lock(m);
                                fwrite(buff,1,strlen(buff),flog);
                                pthread_mutex_unlock(m);
                                queryCount++;
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
                write(fd,"N",1);
                write(fd,0,sizeof(0));
            }
            char str[20];
            if(type=='L'){
                sprintf(str, "LOAN %u\n",queryCount);
            }else{
                sprintf(str, "QUERY %u\n",queryCount);
            }
            pthread_mutex_lock(m);
            fwrite(str,1,strlen(str),flog);
            pthread_mutex_unlock(m);
            close(fd);
            freeBook(book);
            free(buff);
        }
	}
    printf("Thread ID: %ld è terminato\n",pthread_self());
	return NULL;
}

Book_t* recordToBook(char* riga, Book_t* book){
    int n_attributes=countAttributes(riga);
    char* field[n_attributes];
    char* str=strtok(riga,";");
    field[0]=(char*)malloc(strlen(str)+1);
    strcpy(field[0],str);
    for(int i=1;i<n_attributes;i++){
        str = strtok(NULL,";");
        field[i]=(char*)malloc(strlen(str)+1);
        strcpy(field[i],str);
    }
    int n_autori=0;
    NodoAutore* previousAuthor;
    bool firstIteration=true;
    for(int i=0;i<n_attributes;i++){
        char* subfield=strchr(field[i],':')+1+strspn(strchr(field[i],':')+1, " ");
        if(strcmp(subfield,"")!=0){ //sporco key perché tanto il campo è vuoto
            int key_length = strchr(field[i],':') - field[i] - strspn(field[i], " ");
            char key[key_length + 1];
            strncpy(key, field[i] + strspn(field[i], " "), key_length);
            key[key_length] = '\0'; 
            while(key[key_length-1]==' '){
                key_length--;
            } 
            strncpy(key, field[i]+strspn(field[i], " "), key_length);  
            key[key_length]='\0';   
            int value_length=strlen(subfield);
            while(subfield[value_length-1]==' '){
                value_length--;
            }
            char value[value_length+1];
            value[value_length]='\0';
            strncpy(value,subfield,value_length);
            if(strcmp(key,"autore") == 0){
                NodoAutore* currentAuthor=(NodoAutore*)malloc(sizeof(NodoAutore));
                memset(currentAuthor, 0, sizeof(NodoAutore));
                currentAuthor->val=(char*)malloc(strlen(value)+1);
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
        }
    }
    for (int i = 0; i < n_attributes; i++) {
        free(field[i]);
    }
    return book;
}

bool matchElemBook(Book_t* book, Book_t* bookNode){
    bool match=false;
    if(book->autore!=NULL && bookNode->autore!=NULL){
        NodoAutore* currAuthor=bookNode->autore;
        while(currAuthor != NULL && currAuthor->val != NULL){
            if(strcmp(currAuthor->val,book->autore->val)==0){
                match=true;
            }
            currAuthor=currAuthor->next;
        }
    }
    if(book->titolo!=NULL && bookNode->titolo!=NULL){
        if(strcmp(bookNode->titolo,book->titolo)==0){
            match=true;
        }          
    }
    if(book->editore!=NULL && bookNode->editore!=NULL){
        if(strcmp(bookNode->editore,book->editore)==0){
            match=true;
        }          
    }
    if(book->anno!=0 && bookNode->nota!=0){
        if(book->anno==bookNode->anno){
            match=true;
        }          
    }
    if(book->nota!=NULL && bookNode->nota!=NULL){
        if(strcmp(bookNode->nota,book->nota)==0){
            match=true;
        }          
    }
    if(book->collocazione!=NULL && bookNode->collocazione!=NULL){
        if(strcmp(bookNode->collocazione,book->collocazione)==0){
            match=true;
        }          
    }
    if(book->luogo_pubblicazione!=NULL && bookNode->luogo_pubblicazione!=NULL){
        if(strcmp(bookNode->luogo_pubblicazione,book->luogo_pubblicazione)==0){
            match=true;
        }          
    }
    if(book->descrizione_fisica!=NULL && bookNode->descrizione_fisica!=NULL){
        if(strcmp(bookNode->descrizione_fisica,book->descrizione_fisica)==0){
            match=true;
        }          
    }
    return match;
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

static void gestore (int signum) {
    sigflag=-1;
    if(signum==2){
        printf("\nRICEVUTO SEGNALE SIGINT\n");
        printf("Terminazione server\n");
    }else if(signum==15){
        printf("\nRICEVUTO SEGNALE SIGTERM\n");
        printf("Terminazione server\n");
    }else{
        printf("\nRICEVUTO SEGNALE %d\n",signum);
    }
}