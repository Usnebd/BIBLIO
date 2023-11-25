#include"bibserver.h"
#define SIZE 1024
#define UNIX_PATH_MAX 80
#define N 100

volatile int sigflag=0;

int main(int argc, char* argv[]){
    struct sigaction s;
    sigset_t sigmask;
    sigfillset(&sigmask);
    pthread_sigmask(SIG_SETMASK,&sigmask,NULL);
    //blocco tutti i segnali
    memset(&s,0,sizeof(s));
    sigset_t handlerMask;
    sigfillset(&handlerMask);
    s.sa_mask=handlerMask;
    s.sa_handler=gestore; //registro gestore
    sigaction(SIGINT,&s,NULL);
    sigaction(SIGTERM,&s,NULL);
    if(argc!=4){
        printf("Not the right amount of args\n");
        exit(EXIT_FAILURE);
    }
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int n_workers = atoi(argv[3]);
    size_t size=SIZE;    
    FILE* fin = fopen(file_name,"r");
    if(fin){
        addBibToConf(name_bib);
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
                }else if(isPresent(book,head)){
                    freeBook(book);
                    free(elem);
                }else{
                    elem->next=head;
                    head=elem;
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
            exit(EXIT_FAILURE);
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
            pthread_create(&tid,NULL,worker,&threadArgs);
            threadArray[j]=tid;
        }
        rdset=set;
        sigdelset(&sigmask,SIGINT);
        sigdelset(&sigmask,SIGTERM);
        pthread_sigmask(SIG_SETMASK,&sigmask,NULL);
        while(sigflag!=-1){
            if(pselect(fd_num+1,&rdset,NULL,NULL,NULL,&sigmask)!=-1){
                for (int i=0;i<fd_num+1;i++){
                    if (FD_ISSET(i,&rdset)){
                        if (i==welcomeSocket){
                            int fd_c=accept(welcomeSocket, NULL,0);
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
            rdset=set;
	    }
        sigfillset(&sigmask);
        pthread_sigmask(SIG_SETMASK,&sigmask,NULL);
        for(int i=0;i<n_workers;i++){
            int* endWorkerSignal=(int*)malloc(sizeof(int));
            *endWorkerSignal=-1;
            push(q,endWorkerSignal);
        }
        for(int i=0;i<n_workers;i++){
            pthread_join(threadArray[i],NULL);
        }
        pthread_mutex_destroy(&m);
        deleteQueue(q);
        unlink(path);
        fclose(flog);
        fclose(fin);
        deleteFromConf(name_bib);
        dumpRecord(file_name,head);
    }else if(ferror(fin)){
        perror("Errore durante la lettura del file contentente i record\n");
    }  
    printf("\nTerminazione server: %s\n",name_bib);
    return 0;
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
            if(book->volume!=NULL){
                offset +=sprintf(record+offset," volume: %s;",book->volume);
            }
            if(book->luogo_pubblicazione!=NULL){
                offset +=sprintf(record+offset," luogo_pubblicazione: %s;",book->luogo_pubblicazione);
            }
            if(book->scaffale!=NULL){
                offset +=sprintf(record+offset," scaffale: %s;",book->scaffale);
            }
            if(strcmp(book->prestito,"")!=0){
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
        exit(EXIT_FAILURE);
    }  
}

void deleteFromConf(char* name_bib){
    FILE* fconf=fopen("bib.conf","r+");
    size_t size=N;
    char* riga=(char*)malloc(N);
    if(fconf==NULL){
        perror("errore apertura file bib.conf\n");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }else{
        while(!feof(fconf)){
            if((getline(&riga,&size,fconf))>1){
                strtok(riga,",");
                strtok(riga,":");
                char* tok=strtok(NULL,":");
                if(strcmp(tok,name_bib)==0){             
                    perror("nome biblioteca già preso\n");
                    exit(EXIT_FAILURE);
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
    NodoAutore* bookNodeAuthor=bookNode->autore;
    while(bookAuthor!=NULL){
        bool authorPresent=false;
        while(bookNodeAuthor!=NULL){
            if(strcmp(bookNodeAuthor->val, bookAuthor->val)==0){
                authorPresent=true;
                break;
            }
            bookNodeAuthor=bookNodeAuthor->next;
        }
        if(authorPresent==false){
            return false;
        }
        bookAuthor=bookAuthor->next;
        bookNodeAuthor=bookNode->autore;
    }
    return true;
}

bool isPresent(Book_t* book, Elem* head){
    Elem* currElem=head;
    bool result=false;
    while(currElem!=NULL){
        if(matchBook(book,currElem->val)){
            Book_t* bookNode=currElem->val;
            if(bookNode->titolo==NULL){
                if(book->titolo!=NULL){
                    bookNode->titolo=(char*)malloc(strlen(book->titolo)+1);
                    strcpy(bookNode->titolo,book->titolo);
                }
            }
            if(bookNode->anno==0){
                bookNode->anno=book->anno;
            }
            if(bookNode->editore==NULL){
                if(book->editore!=NULL){
                    bookNode->editore=(char*)malloc(strlen(book->editore)+1);
                    strcpy(bookNode->editore,book->editore);
                }
            }
            if(bookNode->nota==NULL){
                if(book->nota!=NULL){
                    bookNode->nota=(char*)malloc(strlen(book->nota)+1);
                    strcpy(bookNode->nota,book->nota);
                }
            }
            if(bookNode->collocazione==NULL){
                if(book->collocazione!=NULL){
                    bookNode->collocazione=(char*)malloc(strlen(book->collocazione)+1);
                    strcpy(bookNode->collocazione,book->collocazione);
                }
            }
            if(bookNode->descrizione_fisica==NULL){
                if(book->descrizione_fisica!=NULL){
                    bookNode->descrizione_fisica=(char*)malloc(strlen(book->descrizione_fisica)+1);
                    strcpy(bookNode->descrizione_fisica, book->descrizione_fisica);
                }
            }
            if(bookNode->volume==NULL){
                if(book->volume!=NULL){
                    bookNode->volume=(char*)malloc(strlen(book->volume)+1);
                    strcpy(bookNode->volume,book->volume);
                }
            }
            if(bookNode->luogo_pubblicazione==NULL){
                if(book->luogo_pubblicazione!=NULL){
                    bookNode->luogo_pubblicazione=(char*)malloc(strlen(book->luogo_pubblicazione)+1);
                    strcpy(bookNode->luogo_pubblicazione,book->luogo_pubblicazione);
                }
            }
            if(bookNode->scaffale==NULL){
                if(book->scaffale!=NULL){
                    bookNode->scaffale=(char*)malloc(strlen(book->scaffale)+1);
                    strcpy(bookNode->scaffale,book->scaffale);
                }
            }
            if(strcmp(bookNode->prestito,"")==0){
                if(strcmp(book->prestito,"")!=0){
                    strcpy(bookNode->prestito,book->prestito);
                }
            }
            result=true;
            break;
        }
        currElem=currElem->next;
    }
    return result;
}

void* worker(void* args){
    Queue_t* q=((arg_t*)args)->q;
    Elem* head=((arg_t*)args)->list;
    pthread_mutex_t* m=((arg_t*)args)->mutex;
    FILE* flog=((arg_t*)args)->flog;
	while(sigflag!=-1){
        int* ftemp=(int*)pop(q);
        int fd=*ftemp;
        free(ftemp);
        if(fd!=-1){
            Elem* node=head;
            unsigned int length;
            char type;
            read(fd,&type,1);
            read(fd,&length,sizeof(unsigned int));
            char* buff = (char*)malloc(length);
            read(fd,buff,length);
            Book_t* book=(Book_t*)malloc(sizeof(Book_t)); 
            memset(book, 0, sizeof(Book_t));
            recordToBook(buff,book);
            bool noMatches;
            bool error;
            int queryCount=0;
            switch(type){
                case MSG_QUERY:
                    noMatches=true;
                    while(node!=NULL){
                        if(matchBook(book,node->val)){
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
                    error=true;
                    while(node!=NULL){
                        if(matchBook(book,node->val)){
                            noMatches=false;
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
                                error=false;
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
                                offset += sprintf(node->val->prestito + offset, "%d", currentTime->tm_year + 1900 + 1); //prestitio dura un anno
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
                error=false;
                unsigned int zero=0;
                write(fd,"N",1);
                write(fd,&zero,sizeof(zero));
            }
            if(error){
                write(fd,"E",1);
                char errorMsg[] = "Errore, nessun libro disponibile";
                unsigned int msg_lenght = strlen(errorMsg)+1;
                write(fd,&msg_lenght,sizeof(unsigned int));
                write(fd,errorMsg,msg_lenght);
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
    pthread_exit(NULL);
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
            char formatted_value[2*value_length];
            strcpy(formatted_value,"");
            char* tokvalue=strtok(value,",");
            int toklength=strlen(tokvalue);
            while(tokvalue[toklength-1]==' '){
                toklength--;
            }
            strncat(formatted_value,tokvalue+strspn(tokvalue," "),toklength-strspn(tokvalue," "));
            char* precTok=(char*)malloc(strlen(tokvalue)+1);
            strcpy(precTok,tokvalue);
            while((tokvalue=strtok(NULL,","))!=NULL){
                toklength=strlen(tokvalue);
                while(tokvalue[toklength-1]==' '){
                    toklength--;
                }
                if(strcmp(precTok,"")!=0){
                    strcat(formatted_value,",");
                }
                if(strspn(tokvalue," ")>1){
                    tokvalue=tokvalue+strspn(tokvalue," ")-1;
                }else if(strspn(tokvalue," ")==0){
                    strcat(formatted_value," ");
                }
                strncat(formatted_value,tokvalue,toklength);
                precTok=(char*)realloc(precTok,toklength+1);
                strcpy(precTok,tokvalue);
            }
            free(precTok);
            if(strcmp(key,"autore") == 0){
                NodoAutore* currentAuthor=(NodoAutore*)malloc(sizeof(NodoAutore));
                memset(currentAuthor, 0, sizeof(NodoAutore));
                currentAuthor->val=(char*)malloc(strlen(formatted_value)+1);
                strcpy(currentAuthor->val,formatted_value);
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
                book->titolo=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->titolo,formatted_value);
            }else if(strcmp(key,"editore") == 0){
                book->editore=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->editore,formatted_value);
            }else if(strcmp(key,"nota") == 0){
                book->nota=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->nota,formatted_value);
            }else if(strcmp(key,"collocazione") == 0){
                book->collocazione=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->collocazione,formatted_value);
            }else if(strcmp(key,"luogo_pubblicazione") == 0){
                book->luogo_pubblicazione=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->luogo_pubblicazione,formatted_value);
            }else if(strcmp(key,"scaffale") == 0){
                book->scaffale=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->scaffale,formatted_value);
            }else if(strcmp(key,"anno") == 0){
                book->anno=atoi(formatted_value);
            }else if(strcmp(key,"prestito") == 0){
                strcpy(book->prestito,formatted_value);
            }else if(strcmp(key,"descrizione_fisica") == 0){
                book->descrizione_fisica=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->descrizione_fisica,formatted_value);
            }else if(strcmp(key,"volume") == 0){
                book->volume=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->volume,formatted_value);
            }  
        }
    }
    for (int i = 0; i < n_attributes; i++) {
        free(field[i]);
    }
    return book;
}

bool matchBook(Book_t* book, Book_t* bookNode){
    int match=0;
    int bookFieldNum=0;
    if(book->autore!=NULL){
        bookFieldNum++;
        if(bookNode->autore!=NULL){
            if(equalAuthors(book,bookNode)){
                match++;
            }
        }
    }
    if(book->titolo!=NULL){
        bookFieldNum++;
        if(bookNode->titolo!=NULL){
            if(strcmp(bookNode->titolo,book->titolo)==0){
                match++;
            } 
        }         
    }
    if(book->editore!=NULL){
        bookFieldNum++;
        if(bookNode->editore!=NULL){
            if(strcmp(bookNode->editore,book->editore)==0){
                match++;
            } 
        }         
    }
    if(book->anno!=0){
        bookFieldNum++;
        if(bookNode->anno!=0){
            if(book->anno==bookNode->anno){
                match++;
            }
        }          
    }
    if(book->nota!=NULL){
        bookFieldNum++;
        if(bookNode->nota!=NULL){
            if(strcmp(bookNode->nota,book->nota)==0){
                match++;
            }          
        }
    }
    if(book->collocazione!=NULL){
        bookFieldNum++;
        if(bookNode->collocazione!=NULL){
            if(strcmp(bookNode->collocazione,book->collocazione)==0){
                match++;
            }  
        }        
    }
    if(book->luogo_pubblicazione!=NULL){
        bookFieldNum++;
        if(bookNode->luogo_pubblicazione!=NULL){
            if(strcmp(bookNode->luogo_pubblicazione,book->luogo_pubblicazione)==0){
                match++;
            }
        }          
    }
    if(book->scaffale!=NULL){
        bookFieldNum++;
        if(bookNode->scaffale!=NULL){
            if(strcmp(bookNode->scaffale,book->scaffale)==0){
                match++;
            } 
        }        
    }
    if(book->descrizione_fisica!=NULL){
        bookFieldNum++;
        if(bookNode->descrizione_fisica!=NULL){
            if(strcmp(bookNode->descrizione_fisica,book->descrizione_fisica)==0){
                match++;
            } 
        }       
    }
    if(book->volume!=NULL){
        bookFieldNum++;
        if(bookNode->volume!=NULL){
            if(strcmp(bookNode->volume,book->volume)==0){
                match++;
            } 
        }         
    }
    if(bookFieldNum>match){
        return false;
    }else{
        return true;
    }
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
}
