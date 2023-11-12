#include"bibserver.h"

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
        FILE* flog=fopen(strcat(name_bib,".log"),"w");
        //aggiungere if(flog)
        struct sockaddr_un sa_server;
        struct sockaddr sa_client;
        strncpy(sa_server.sun_path,name_bib,UNIX_PATH_MAX);
        sa_server.sun_family=AF_UNIX;
        int welcomeSocket=socket(AF_UNIX,SOCK_STREAM,0);
        bind(welcomeSocket,(struct sockaddr*)&sa_server,sizeof(sa_server));
        listen(welcomeSocket,SOMAXCONN);
    
        fd_set set, rdset;
        FD_ZERO(&set);
        FD_SET(welcomeSocket,&set);
        int fd_num=0, fd_c,fdMax=welcomeSocket, nread;
        if (welcomeSocket > fd_num) fd_num = welcomeSocket;
        pthread_mutex_t m;
        pthread_mutex_init(&m,NULL);
        Queue_t* q= initQueue();
        arg_t threadArgs;
        threadArgs.q=q;
        threadArgs.mutex=&m;
        threadArgs.fdMax=&fdMax;
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
            int currMax=fdMax;
            pthread_mutex_unlock(&m);
            select(currMax+1,&rdset,NULL,NULL,NULL);
            pthread_mutex_lock(&m);
            currMax = fdMax;
            pthread_mutex_unlock(&m);
            for (int i=0;i<currMax+1;i++){
                if (FD_ISSET(i,&rdset)){
                    if (i==welcomeSocket){
                        fd_c=accept(welcomeSocket, NULL,NULL);
                        printf("E' arrivato un cliente \n");
                        pthread_mutex_lock(&m);
                        FD_SET(fd_c,&set);
                        if(fd_c>fdMax)
                            fdMax=fd_c;
                        pthread_mutex_unlock(&m);
                    }else{
                        int* client=malloc(sizeof(int));
                        *client=i;
                        push(q,client);
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
    size_t size=N;
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
    strtok(riga,";");
    while(strtok(NULL,";")!=NULL){
        count++;
    }
    return count;
}

Book_t* recordToBook(char* riga, Book_t* book){
    char* unfiltered_key;
    char* value;
    char temp_riga[strlen(riga)];
    strcpy(temp_riga,riga);
    int n_attributes=countAttributes(temp_riga);
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
    NodoAutore* head;
    bool firstIteration=true;
    for(int i=0;i<n_attributes;i++){
        unfiltered_key = (char*)malloc(100);
        value = (char*)malloc(100);
        char temp_field[strlen(field[i])];
        strcpy(temp_field,field[i]);
        strcpy(unfiltered_key,strtok(temp_field,":"));
        char* tok_value=strtok(NULL,":");
        strcat(value,tok_value);
        tok_value=strtok(NULL,":");
        while((tok_value!=NULL)){
            strcat(value,":");
            strcat(value,tok_value);
            tok_value=strtok(NULL,":");
        }
        char* filtered_key=strtok(unfiltered_key," ");
        value=value+1;
        if(strcmp(filtered_key,"autore") == 0){
            NodoAutore* node=(NodoAutore*)malloc(sizeof(NodoAutore));
            node->val=(char*)malloc(strlen(value));
            strcpy(node->val,value);
            if(firstIteration){
                head=node;
                firstIteration=false;
            }else{
                node->next=head;
                head=node;
            }
            n_autori++;
        }else if(strcmp(filtered_key,"titolo") == 0){
            book->titolo=(char*)malloc(strlen(value));
            strcpy(book->titolo,value);
        }else if(strcmp(filtered_key,"editore") == 0){
            book->editore=(char*)malloc(strlen(value));
            strcpy(book->editore,value);
        }else if(strcmp(filtered_key,"nota") == 0){
            book->nota=(char*)malloc(strlen(value));
            strcpy(book->nota,value);
        }else if(strcmp(filtered_key,"collocazione") == 0){
            book->collocazione=(char*)malloc(strlen(value));
            strcpy(book->collocazione,value);
        }else if(strcmp(filtered_key,"luogo_pubblicazione") == 0){
            book->luogo_pubblicazione=(char*)malloc(strlen(value));
            strcpy(book->luogo_pubblicazione,value);
        }else if(strcmp(filtered_key,"anno") == 0){
            book->anno=atoi(value);
        }else if(strcmp(filtered_key,"prestito") == 0){
            book->prestito=(char*)malloc(strlen(value));
            strcpy(book->prestito,value);
        }else if(strcmp(filtered_key,"descrizione_fisica") == 0){
            book->descrizione_fisica=(char*)malloc(strlen(value));
            strcpy(book->descrizione_fisica,value);
        }
        book->autore=(NodoAutore*)malloc(sizeof(NodoAutore));
        *book->autore=*head;
        free(unfiltered_key);
        value=value-1;
        free(value);
    }
    return book;
}

void* worker(void* args){
    time_t timestamp = time(NULL);
    struct tm* data = localtime(&timestamp);
    //prendo la data
    Queue_t* q=((arg_t*)args)->q;
	int* fdMax=((arg_t*)args)->fdMax;
	fd_set* clients=((arg_t*)args)->clients;
    Elem* head=((arg_t*)args)->list;
    Elem node=*head;
	pthread_mutex_t* m=((arg_t*)args)->mutex;
	char* buff=(char*)malloc(sizeof(unsigned int));
    unsigned int length;
	while(1){
		int fd=*(int*)pop(q);
        char type;
		read(fd,&type,1);
        read(fd,&length,sizeof(unsigned int));
        free(buff);
        buff=(char*)malloc(length);
        read(fd,buff,length);
        Book_t* book;
        recordToBook(buff,book);
        free(buff);
        bool msgReturnType=false;
        switch(type){
            case MSG_QUERY:
                while(node.val!=NULL){
                    if(matchElemBook(book,node.val)==true){
                        buff=(char*)malloc(SIZE);
                        bookToRecord(node.val,buff,MSG_QUERY);
                        msgReturnType=true;
                        write(fd,"R",1);
                        unsigned int bufflength=strlen(buff);
                        write(fd,&bufflength,sizeof(size_t)); 
                        write(fd,buff,strlen(buff));
                    }
                    if(node.next==NULL){
                        node.val=NULL;
                    }else{
                        node=*(node.next);
                    }
                }
                if(msgReturnType==false){
                    write(fd,"N",1);
                    write(fd,0,sizeof(0));
                }
                break;
            case MSG_LOAN:
                while(node.val!=NULL){
                    if(matchElemBook(book,node.val)==true){
                        strcpy(node.val->prestito,"");
                        node.val->prestito[strlen(node.val->prestito)]=data->tm_mday;
                        strcat(node.val->prestito,"-");
                        node.val->prestito[strlen(node.val->prestito)]=data->tm_mon;
                        strcat(node.val->prestito,"-");
                        node.val->prestito[strlen(node.val->prestito)]=data->tm_year;
                        buff=(char*)malloc(SIZE);
                        bookToRecord(node.val,buff,MSG_LOAN);
                        msgReturnType=true;
                        write(fd,"R",1);
                        unsigned int bufflength=strlen(buff);
                        write(fd,&bufflength,sizeof(size_t)); 
                        write(fd,buff,strlen(buff));
                    }
                    node=*(node.next);
                }
                if(msgReturnType==false){
                    write(fd,"N",1);
                    write(fd,0,sizeof(0));
                }
                break;
            default:
                break;
        }
		close(fd);
	}
	return NULL;
}

bool matchElemBook(Book_t* book, Book_t* bookNode){
    bool match=true;
    int length=0;
    while(bookNode->autore->val!=NULL){
        if(strcmp(bookNode->autore->val,book->autore->val)!=0){
            match=false;
        }else{
            match=true;
        }
        bookNode->autore=bookNode->autore->next;
    }
    if(book->titolo!=NULL){
        if(strcmp(bookNode->titolo,book->titolo)!=0){
            match=false;
        }          
    }
    if(book->editore!=NULL){
        if(strcmp(bookNode->editore,book->editore)!=0){
            match=false;
        }          
    }
    if(book->anno!=0){
        if(book->anno!=bookNode->anno){
            match=false;
        }          
    }
    if(book->nota!=NULL){
        if(strcmp(bookNode->nota,book->nota)!=0){
            match=false;
        }          
    }
    if(book->collocazione!=NULL){
        if(strcmp(bookNode->collocazione,book->collocazione)!=0){
            match=false;
        }          
    }
    if(book->luogo_pubblicazione!=NULL){
        if(strcmp(bookNode->luogo_pubblicazione,book->luogo_pubblicazione)!=0){
            match=false;
        }          
    }
    if(book->descrizione_fisica!=NULL){
        if(strcmp(bookNode->descrizione_fisica,book->descrizione_fisica)!=0){
            match=false;
        }          
    }
    return match;
}