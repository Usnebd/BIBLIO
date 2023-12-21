#include"bibserver.h"
#define SIZE 1024
#define UNIX_PATH_MAX 80
#define N 100

volatile int sigflag=0;         //variabile settata dal gestore dei segnali SIGINT e SIGTERM

int main(int argc, char* argv[]){
    struct sigaction s;
    sigset_t sigmask;       //creazione maschera 
    sigfillset(&sigmask);   //setto tutti i bit della maschera
    pthread_sigmask(SIG_SETMASK,&sigmask,NULL);      //blocco tutti i segnali

    memset(&s,0,sizeof(s)); 
    sigset_t handlerMask;                 //creo la maschera che passerò a sigaction
    sigfillset(&handlerMask);           
    s.sa_mask=handlerMask;                //quando eseguirò il gestore mi baserò sulla handlerMask per capire quali segnali mascherare
    s.sa_handler=gestore; //registro gestore
    sigaction(SIGINT,&s,NULL);
    sigaction(SIGTERM,&s,NULL);
    if(argc!=4){
        printf("Not the right amount of args\n");
        exit(EXIT_FAILURE);
    }
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int n_workers = atoi(argv[3]);              //leggo i parametri
    size_t size=SIZE;    
    FILE* fin = fopen(file_name,"r");           //apro il file record
    if(fin){
        addBibToConf(name_bib);                 //aggiungo l'indirizzo del server al file bib.conf
        Elem* head=NULL;
        bool firstIteration=true;
        int nchar=0;
        char* riga=(char*)malloc(SIZE);
        while((nchar=getline(&riga,&size,fin))!=-1){        //finchè getline non è arrivato in fondo al file
            if(nchar!=0 && nchar!=1){                       //se non è una riga vuota 
                Book_t* book=(Book_t*)malloc(sizeof(Book_t));       //alloco memoria per il libro
                memset(book, 0, sizeof(Book_t));                    //pulisco la memoria dei campi attualmente vuoti
                recordToBook(riga,book);                            //leggo la riga e la traduco in libro
                Elem* elem=(Elem*)malloc(sizeof(Elem));             //alloco memoria per il nodo della lista
                elem->val=book;                                     //assegno il riferimento al libro al valore del nodo
                if(firstIteration){                                 //se è la prima 
                    head=elem;
                    firstIteration=false;
                }else if(isPresent(book,head)){                     //se il libro è già presente nella lista
                    freeBook(book);                                 //dealloco la memoria di tutto il libro
                    free(elem);                                 
                }else{
                    elem->next=head;                                //altrimenti faccio puntare il nodo precedente
                    head=elem;                                      //la testa punta al nodo attuale
                }
            }
        }
        fclose(fin);                                                //chiudo il file record, adesso non mi serve più
        free(riga);
        char path[strlen(name_bib)+1];                              //costruisco la stringa che contiene l'indirizzo
        char logName[strlen(name_bib)+strlen(".log")];              //costruisco la stringa che rappresenta il nome del file di log
        sprintf(path,"./%s",name_bib);
        sprintf(logName,"%s.log",name_bib);
        FILE* flog=fopen(logName,"w");                              //apro in scrittura il file di log
        checkFerror(flog);                                          //controllo errori
        struct sockaddr_un sa_server;
        unlink(path);                                               //se c'è una soscket o file con lo stesso nome di path lo rimuovo
        strncpy(sa_server.sun_path,path,UNIX_PATH_MAX);             //copio il path sulla struct
        sa_server.sun_family=AF_UNIX;                               //imposto il dominio AF_UNIX alla struct sockaddr_un
        int welcomeSocket=socket(AF_UNIX,SOCK_STREAM,0);            //creo il socket connection-oriented
        bind(welcomeSocket,(struct sockaddr*)&sa_server,sizeof(sa_server)); //associo il socket ad un indirizzo
        listen(welcomeSocket,SOMAXCONN);                            //segnalo che il socket accetta connessioni

        fd_set set, rdset;      //creo un fd_set che conterrà l'insieme dei file descriptor da tener traccia e un di quelli pronti alla lettura
        FD_ZERO(&set);          //azzero il set
        FD_SET(welcomeSocket,&set);     //imposto la socket del server nel set
        int fd_num=0;
        if (welcomeSocket > fd_num) fd_num = welcomeSocket;    //mantengo il massimo indice di descrittore attivo in fd_num
        Queue_t* q=initQueue();                                //creo la coda di priorità
        pthread_mutex_t m;                                     //creo il mutex
        pthread_mutex_init(&m,NULL);
        arg_t threadArgs;
        threadArgs.q=q;
        threadArgs.list=head;
        threadArgs.mutex=&m;
        threadArgs.flog=flog;
        pthread_t threadArray[n_workers];
        for(int j=0;j<n_workers;j++){                         //avvio la threadPool
            pthread_t tid;
            pthread_create(&tid,NULL,worker,&threadArgs);
            threadArray[j]=tid;
        }
        rdset=set;                                            //imposto il set di file pronti alla lettura
        sigdelset(&sigmask,SIGINT);                           //rimuovo dal set di segnali da ignorare SIGINT
        sigdelset(&sigmask,SIGTERM);                          //rimuovo dal set di segnali da ignorare SIGTERM
        pthread_sigmask(SIG_SETMASK,&sigmask,NULL);           //aggiorno la maschera riattivando così la ricezione dei segnali SIGINT e SIGTERM
        while(sigflag!=-1){                                   //finchè la variabile sigflag non è stata settata a -1 dall'handler dei segnali
            if(pselect(fd_num+1,&rdset,NULL,NULL,NULL,&sigmask)!=-1){       //controllo se ci sono fd pronti alla lettura nel rdset di dimensione fd_num+1
                for (int i=0;i<fd_num+1;i++){                               //passo a pselect sigmask perché voglio che pselect venga interrotta solo dai segnali SIGINT e SIGTERM
                    if (FD_ISSET(i,&rdset)){                                //se il fd è settato a 1 in rdset
                        if (i==welcomeSocket){                              //se è il welcomeSocket
                            int fd_c=accept(welcomeSocket, NULL,0);         //accetto la connessione    
                            FD_SET(fd_c,&set);                              //aggiungo il fd del client al set di fd
                            if(fd_c>fd_num)                                 //aggiorno il fd MAX
                                fd_num=fd_c;
                        }else{  
                            int* client=(int*)malloc(sizeof(int));                //mi creo un riferimento al fd del client
                            *client=i;                                                      
                            push(q,client);                                       //lo pusho nella coda
                            FD_CLR(i,&set);                                       //lo tolgo dal set perché da adesso in poi verrà gestito dal client
                            fd_num=aggiornaMax(set,fd_num);      
                        }
                    }
                }
            }
            rdset=set;                                             //aggiorno rdset
	    }
        sigfillset(&sigmask);                                     //blocco TUTTI I SEGNALI
        pthread_sigmask(SIG_SETMASK,&sigmask,NULL);               //aggiorno la maschera
        for(int i=0;i<n_workers;i++){                             
            int* endWorkerSignal=(int*)malloc(sizeof(int));
            *endWorkerSignal=-1;                                
            push(q,endWorkerSignal);                              //inserisco dei -1 nella coda per segnalare ai workers di terminare l'esecuzione
        }
        for(int i=0;i<n_workers;i++){
            pthread_join(threadArray[i],NULL);                   //eseguo la join
        }
        pthread_mutex_destroy(&m);                              //distruggo il mutex usato dai worker eper scrivere nel file di log
        deleteQueue(q);                                         //elimino la coda
        unlink(path);                                           //rimuovo il nome della socket del server dal file system
        fclose(flog);                                           //chiudo il file di log                                    
        deleteFromConf(name_bib);                               //cancello l'indizzo dal file di configurazione
        dumpRecord(file_name,head);                             //faccio il dump della lista contenente i libri sul file record
    }else{
        checkFerror(fin);
    }  
    printf("\nTerminazione server: %s\n",name_bib);
    return 0;
}

void dumpRecord(char* filename, Elem* head){
    FILE* fin=fopen(filename,"w");                                  //apto il file record in scrittura troncandolo
    if(fin){    
        ftruncate(fileno(fin), 0);                                  //pulisco il file di record
        char* record=(char*)malloc(SIZE);
        int offset;
        NodoAutore* nodeAuthor;
        Book_t* book;
        while(head!=NULL){                          //finchè ci sono nodi nella lista di libri
            bool firstIteration=true;
            offset=0;
            book=head->val;
            nodeAuthor=book->autore;
            while(nodeAuthor!=NULL){               //finchè ci sono nodi nella lista di autori    
                if(nodeAuthor->val!=NULL){         //se il campo val non è NULL
                    if(firstIteration){            
                        firstIteration=false;
                        offset +=sprintf(record,"autore: %s;",nodeAuthor->val);
                    }else{                          //se non è la prima iterazione includo lo spazio prima del nome del campo
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
            strcat(record,"\n");        //aggiungo il char di fine riga
            offset++;
            fwrite(record+strspn(record," "),1,offset,fin);         //scrivo il record nel file record
            checkFerror(fin);
            Elem* tmp=head;
            head=head->next;
            freeBook(tmp->val);                                     //dealloco il libro
            free(tmp);                                              //dealloco il nodo della lista
        }   
        free(record);                                   
        fclose(fin);                                                //chiudo il file
    }else{
        checkFerror(fin);
    }
}

void deleteFromConf(char* name_bib){
    FILE* fconf=fopen("bib.conf","r+");                         //apro il file di configurazione
    size_t size=N;
    char* riga=(char*)malloc(N);
    if(fconf){
        int nchar=0;
        long pos_inizio = 0;
        long pos_fine = 0;
        while((nchar=getline(&riga,&size,fconf))!=-1){          //finchè non sono arrivato all fine del file
            if(nchar != 1 && nchar != 0){                       //se non è una riga vuota o un carattere di nuova riga
                pos_inizio = pos_fine;                          
                pos_fine = ftell(fconf);                        //mi salvo la posizione di inzio riga
                if (strstr(riga, name_bib) != NULL) {           //se la riga contiene la sottostringa contenente l'indirizzo 
                    fseek(fconf, pos_inizio, SEEK_SET);         //imposto la posizione all'inizio della riga 
                    char tmp[strlen(riga)];
                    char tmpName[strlen(name_bib)];
                    memset(tmpName,'X',strlen(name_bib));       
                    tmpName[strlen(name_bib)]='\0';
                    sprintf(tmp,"BibName:%s,Sockpath:%s\n",tmpName,tmpName);   //aggiungo la stringa XXXXXXXX 
                    fwrite(tmp, 1, strlen(riga), fconf);                       //scrivo la riga
                    checkFerror(fconf);
                    fseek(fconf, 0, SEEK_END);                                 //vado alla fine del file così la getline si restituisce -1
                }
                memset(riga,0,N);
            }
        }
    }else{
        free(riga);
        checkFerror(fconf);
    }
    fclose(fconf);                                  //chiudo file record aggiornato
    free(riga);                                         
}

void addBibToConf(char* name_bib){          
    FILE* fconf=fopen("bib.conf","r+");
    size_t size=N;
    char* riga=(char*)malloc(N);
    if(fconf){
        while(!feof(fconf)){                                //finché non sono alla fine del file
            if((getline(&riga,&size,fconf))>1){             //finchè getline legge qualcosa
                strtok(riga,",");
                strtok(riga,":");
                char* tok=strtok(NULL,":");                 //tokenizzo la riga letta dal file bib.conf
                if(strcmp(tok,name_bib)==0){                //vedo se la riga contiene l'indirizzo del mio server, dunque se è già stato preso
                    perror("nome biblioteca già preso\n");
                    exit(EXIT_FAILURE);
                }
                memset(riga,0,N);
            }
        }
        fprintf(fconf,"BibName:%s,Sockpath:%s\n",name_bib,name_bib);        //aggiungo l'indirizzo del server a bib.conf
    }else if(ferror(fconf)){
        free(riga);
        perror("errore apertura file bib.conf\n");
        exit(EXIT_FAILURE);
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
            if(strcmp(bookNodeAuthor->val, bookAuthor->val)==0){        //se l'autore corrente del libro è presente nella lista di autori del libro della lista
                authorPresent=true;                                     //setto la flag
                break;                                                  //interrompo il ciclo
            }
            bookNodeAuthor=bookNodeAuthor->next;                        //passo all'autore successivo
        }
        if(authorPresent==false){                              //se è false allora vuol dire che l'autore del libro corrente non è contenuto nel libro del nodo
            return false;
        }
        bookAuthor=bookAuthor->next;              //passo all'autore successivo del libro corrente e scorro di nuovo tutta la lista degli autori del libro nel lista del server
        bookNodeAuthor=bookNode->autore;          //resetto il puntantore della lista degli autori alla testa
    }
    return true;
}

bool isPresent(Book_t* book, Elem* head){           
    Elem* currElem=head;
    bool result=false;
    while(currElem!=NULL){                                  //finchè il nodo della lista non è NULL 
        if(matchBook(book,currElem->val)){                  //se c'è un match tra il libro che devo inserire e il libro nel nodo corrente
            Book_t* bookNode=currElem->val;                 //se sono uguali allora procedo ad arricchire il libro nel nodo con i campi del libro che volevo inserire
            if(bookNode->titolo==NULL){                     //se il libro presente nella lista non ha un titolo
                if(book->titolo!=NULL){                     //se il libro che voglio inserire ha un titolo
                    bookNode->titolo=(char*)malloc(strlen(book->titolo)+1);         //copio il titolo nel libro nella lista
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
    Elem* head=((arg_t*)args)->list;                        //leggo e mi salvo i riferimenti agli oggetti di threadArgs
    pthread_mutex_t* m=((arg_t*)args)->mutex;
    FILE* flog=((arg_t*)args)->flog;
	while(sigflag!=-1){                                     //finchè sigflag non è stata settata dall'arrivo di un segnale SIGINT o SIGTERM
        int* ftemp=(int*)pop(q);
        int fd=*ftemp;
        free(ftemp);                                        //mi salvo il file descriptor e dealloco
        if(fd!=-1){                                         //se è "-1" allora il main thread vuole terminare i worker
            Elem* node=head;                                
            unsigned int length;
            char type;
            checkSyscall(read(fd,&type,1));                               //leggo il type del msg
            checkSyscall(read(fd,&length,sizeof(unsigned int)));          //leggo la lunghezza del msg
            char* buff = (char*)malloc(length);
            checkSyscall(read(fd,buff,length));                           //leggo il campo data del msg
            Book_t* book=(Book_t*)malloc(sizeof(Book_t));   //creo un libro 
            memset(book, 0, sizeof(Book_t));                //azzero tutti i campi così se non li inizializzo hanno un valore nullo invece di randomico
            recordToBook(buff,book);                        //converto il campo data mandato dal client in un libro con quelle caratteristiche
            bool noMatches;                                 //flag che mi serve per sapere se la ricerca ha prodotto match con libri nella biblioteca
            bool error;                                     //flag che mi serve per capire se tutti libri che il cliente vuole prendere in prestito non è disponibile
            int queryCount=0;
            switch(type){
                case MSG_QUERY:                             //se type è "Q"
                    noMatches=true;                         //inizializzo noMatches
                    while(node!=NULL){                      //scorro la lista
                        if(matchBook(book,node->val)){      //prendo il valore del nodo corrente e lo confronto con il libro creato dalla query del client 
                            buff=(char*)realloc(buff,SIZE);
                            memset(buff,0,SIZE);            //se c'è stato un match, ovvero il libro del nodo contiene valori uguali a quelli specificati dal client
                            noMatches=false;                //allora converto il libro in riga testuale e setto la flag noMatches
                            checkSyscall(write(fd,"R",1));  //scrivo al client il tipo del msg
                            unsigned int dataLength=bookToRecord(node->val,buff);
                            checkSyscall(write(fd,&dataLength,sizeof(unsigned int)));     //scrivo al client la lunghezza del record 
                            checkSyscall(write(fd,buff,dataLength));                      //scrivo al client il record
                            strcat(buff,"\n");  
                            pthread_mutex_lock(m);                          //il thread corrente cerca di acquisire la mutex 
                            fwrite(buff,1,strlen(buff),flog);               //scrive il record nel file di log
                            checkFerror(flog);
                            pthread_mutex_unlock(m);                        //rilascia la mutex
                            queryCount++;                                   //incrementa il conteggio delle query restituite 
                        }
                        if(node->next!=NULL){                               //passa al nodo successivo della lista
                            if(node->next->val!=NULL){
                                node=node->next;
                            }
                        }else{
                            node=NULL;
                        }
                    }
                    break;
                case MSG_LOAN:                              //se type è "L"
                    noMatches=true;
                    error=true;                             //inizializzo la flag error (nessun libro disponibile al prestito)
                    while(node!=NULL){
                        if(matchBook(book,node->val)){
                            noMatches=false;
                            struct tm loanTime;                                     //loanTime contiene le informazioni sul tempo del prestito 
                            bool available=false;
                            time_t timestamp = time(NULL);
                            struct tm* currentTime = localtime(&timestamp);         //currentTime contiene le informazioni sul tempo corrente
                            if(strcmp(node->val->prestito,"")!=0){                  //il campo prestito del libro della lista NON è null
                                char date[strlen(node->val->prestito)];
                                strcpy(date,node->val->prestito);
                                char* tokDMY=strtok(date," ");  //DMY = Day Month Year
                                char* tokSMH=strtok(NULL," ");      //SMH = Seconds Minutes Hours
                                
                                loanTime.tm_mday=atoi(strtok(tokDMY,"-"));          //aggiungo le informazioni sulla data nella struct loanTime
                                loanTime.tm_mon=atoi(strtok(NULL,"-")-1);
                                loanTime.tm_year=atoi(strtok(NULL,"-"))-1900;

                                loanTime.tm_hour=atoi(strtok(tokSMH,":"));
                                loanTime.tm_min=atoi(strtok(NULL,":"));
                                loanTime.tm_sec=atoi(strtok(NULL,":"));
                                                                                        //confronto le due struct
                                if(difftime(mktime(currentTime),mktime(&loanTime))>0){  //data di scadenza (loanTime) è passata
                                    available=true;
                                }
                            }else{                              //se prestito di node->val è NULL allora il libro è disponibile
                                available=true;    
                            }
                            if(available){
                                error=false;                    //set error false
                                memset(node->val->prestito, 0, sizeof(node->val->prestito));

                                int offset = 0;  // Inizializza l'offset a 0

                                if(currentTime->tm_mday<10){                    //aggiorno il campo prestito del libro nella lista
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
                                unsigned int dataLength=bookToRecord(node->val,buff);   //converto il libro in stringa e mi faccio restituire la lunghezza
                                checkSyscall(write(fd,"R",1));                                        //invio il type del msg
                                checkSyscall(write(fd,&dataLength,sizeof(unsigned int)));             //invio la lunghezza del msg
                                checkSyscall(write(fd,buff,dataLength));                              //invio il campo data del msg
                                strcat(buff,"\n");
                                pthread_mutex_lock(m);                                  //acquisisco la mutex per scrivere nel file log
                                fwrite(buff,1,strlen(buff),flog);                       //scrivo il record
                                checkFerror(flog);
                                pthread_mutex_unlock(m);                                //rilascio la mutex
                                queryCount++;                                           //aumento il numero di riscontri 
                            }                        
                        }
                        if(node->next!=NULL){                                           //passo al nodo successivo
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
            if(noMatches){                          //se non è stata settata la flag noMatches allora non ci sono stati riscontri
                error=false;                        //non voglio entrare nel if successivo
                unsigned int zero=0;
                checkSyscall(write(fd,"N",1));                    //invio il type "N" al client
                checkSyscall(write(fd,&zero,sizeof(zero)));       //invio l'intero 0
            }
            if(error){                              //il client ha provato a richieder un loan ma nessun libro era disponibile oppure il libro non esiste
                checkSyscall(write(fd,"E",1));                    //invio il type "E" al client
                char errorMsg[] = "Errore, nessun libro disponibile";       //data
                unsigned int msg_lenght = strlen(errorMsg)+1;       
                checkSyscall(write(fd,&msg_lenght,sizeof(unsigned int)));             //invio la dimensione della stringa di errore
                checkSyscall(write(fd,errorMsg,msg_lenght));                          //invio la stringa di errore
            }
            char str[20];
            if(type=='L'){      
                sprintf(str, "LOAN %u\n",queryCount);
            }else{                                                      //preparo la stringa da scrivere nel file di log
                sprintf(str, "QUERY %u\n",queryCount);
            }
            pthread_mutex_lock(m);                                      //acquisisco la mutex
            fwrite(str,1,strlen(str),flog);                             //scrivo nel file log
            checkFerror(flog);
            pthread_mutex_unlock(m);                                    //rilascio la mutex
            close(fd);                                                  //chiudo il file descriptor del client ovvero la sua socket
            freeBook(book);                                             //dealloco il libro
            free(buff);             
        }
	}   
    pthread_exit(NULL);                                                 //termino il thread con la funzione apposita
}

Book_t* recordToBook(char* riga, Book_t* book){
    int n_attributes=countAttributes(riga);                 //conto il numero di campi del record
    char* field[n_attributes];                              //creo un array di stringhe
    char* str=strtok(riga,";");                             //puntatore al primo campo
    field[0]=(char*)malloc(strlen(str)+1);                  
    strcpy(field[0],str);                                   //salvo il primo campo    esempio:  "autore: Giovanni" 
    for(int i=1;i<n_attributes;i++){                        //itero per tutti i campi
        str = strtok(NULL,";");
        field[i]=(char*)malloc(strlen(str)+1);
        strcpy(field[i],str);
    }
    int n_autori=0;
    NodoAutore* previousAuthor;
    bool firstIteration=true;
    for(int i=0;i<n_attributes;i++){                        //itero per tutti i campi
        char* subfield=strchr(field[i],':')+1+strspn(strchr(field[i],':')+1, " ");  
        if(strcmp(subfield,"")!=0){ //sporco key perché tanto il campo è vuoto
            int key_length = strchr(field[i],':') - field[i] - strspn(field[i], " ");
            char key[key_length + 1];
            strncpy(key, field[i] + strspn(field[i], " "), key_length);
            key[key_length] = '\0'; 
            while(key[key_length-1]==' '){                  //conto il numero di spazi in fondo alla stringa del nome del campo
                key_length--;
            } 
            strncpy(key, field[i]+strspn(field[i], " "), key_length);  
            key[key_length]='\0';   
            int value_length=strlen(subfield);
            while(subfield[value_length-1]==' '){           //conto il numero di spazi in fondo alla stringa del valore del campo
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
            while((tokvalue=strtok(NULL,","))!=NULL){               //gestione del caso in cui il valore del campo è "  Marco   ,    giallini   "
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
            }                                                   //"  autore  :  Mirco  ,   Angeli  ;"   -------> key="autore"  formatted_value="Mirco, Angeli"
            free(precTok);
            if(strcmp(key,"autore") == 0){                      //il nome del campo corrisponde a "autore"
                NodoAutore* currentAuthor=(NodoAutore*)malloc(sizeof(NodoAutore));          //creo un nodoAutore
                memset(currentAuthor, 0, sizeof(NodoAutore));
                currentAuthor->val=(char*)malloc(strlen(formatted_value)+1);            
                strcpy(currentAuthor->val,formatted_value);                                 //salvo il nome dell'autore nel campo val del nodo
                if(firstIteration){
                    previousAuthor=currentAuthor;                    //il riferimento al nodo precendente è quello attuale (ci serve per la prossima iterazione)
                    book->autore=currentAuthor;                      //il campo autore contiene un riferimento al nodo della lista di autori
                    firstIteration=false;
                }else{
                    previousAuthor->next=currentAuthor;              //il nodo precendente punta a quello attuale
                    previousAuthor=currentAuthor;                    //il nodo attuale diventa il nodo precendente per l'iterazione successiva
                }                                                    //facciamo questo perché vogliamo aggiungere in coda
                n_autori++;                                          //incremento il numero di autori nella lista
            }else if(strcmp(key,"titolo") == 0){                     
                book->titolo=(char*)malloc(strlen(formatted_value)+1);
                strcpy(book->titolo,formatted_value);               //se key="titolo" allora copio il valore del titolo nel campo della struct libro
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
        free(field[i]);                         //faccio la free dell'array di stringhe contente i campi del record
    }
    return book;                                //restituisco il puntatore al libro
}

bool matchBook(Book_t* book, Book_t* bookNode){         //verifico se due libri contengono gli stessi valori nei campi non nulli
    int match=0;
    int bookFieldNum=0;
    if(book->autore!=NULL){                             //il campo autore non è nullo
        bookFieldNum++;
        if(bookNode->autore!=NULL){                     //il campo autore non è nullo
            if(equalAuthors(book,bookNode)){            //hanno gli stessi autori
                match++;
            }
        }
    }
    if(book->titolo!=NULL){
        bookFieldNum++;
        if(bookNode->titolo!=NULL){
            if(strcmp(bookNode->titolo,book->titolo)==0){       //confronto le stringhe dei titoli
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
    if(bookFieldNum>match){     //se non ci sono stati tanti match quanto il numero di campi non nulli
        return false;           //es: bibclient --titolo="Harry Potter" --anno="1997" 
    }else{                      //se bookNode->titolo=NULL  bookNode->anno=1997  ------> bookFieldNum=2  MATCH=1
        return true;           
    }
}

int countAttributes(char* riga){            //conto i campi della riga
    int count=0;
    for(int i=0;i<strlen(riga);i++){        
        if(riga[i]==';'){
            count++;
        }
    }
    return count;
}

static void gestore (int signum) {      //handler SIGINT/SIGTERM
    sigflag=-1;                         //imposto la sigflag per segnalare che è arrivata una signal di SIGINT o SIGTERM
}
