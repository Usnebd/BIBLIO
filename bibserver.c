#include"bibserver.h"
#define SIZE 400
#define UNIX_PATH_MAX 80

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
                Book_t* book;
                get_record(riga,book);
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
        FILE* flog=fopen(strcat(strtok(name_bib,".txt"),".log"),"w");

        struct sockaddr_un sa_server;
        struct sockaddr sa_client;
        strncpy(sa_server.sun_path,name_bib,UNIX_PATH_MAX);
        sa_server.sun_family=AF_UNIX;
        int server=socket(AF_UNIX,SOCK_STREAM,0);
        bind(server,(struct sockaddr*)&sa_server,sizeof(sa_server));
        listen(server,SOMAXCONN);
        fd_set allFDs, readFDs;
        FD_ZERO(&allFDs);
        FD_SET(server,&allFDs);
        int fdMax=server;
        pthread_mutex_t m;
        pthread_mutex_init(&m,NULL);
        Queue_t* q= initQueue();

        arg_t threadArgs;
        threadArgs.q=q;
        threadArgs.mutex=&m;
        threadArgs.fdMax=&fdMax;
        threadArgs.clients=&allFDs;

        for(int j=0;j<n_workers;j++){
            pthread_t tid;
            pthread_create(&tid,NULL,worker,&threadArgs);
            pthread_detach(tid);
        }

        if(ferror(flog)){
            perror("Errore durante la lettura del file di log\n");
        }
        if(ferror(fin)){
            perror("Errore durante la lettura del file contentente i record\n");
        }
        fclose(flog);
        fclose(fin);
    }else{
        perror("Errore apertura file");
    }
    _exit(EXIT_SUCCESS);
}

void aggiornaMax(fd_set set, int* max ){
	while(!FD_ISSET(*max,&set)) *max--;
}

int countAttributes(char* riga){
    int count=0;
    strtok(riga,";");
    while(strtok(NULL,";")!=NULL){
        count++;
    }
    return count;
}

Book_t* get_record(char* riga, Book_t* book){
    book->prestito=NULL;
    char* unfiltered_key;
    char* unfiltered_value;
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
    for(int i=0;i<n_attributes;i++){
        unfiltered_key = (char*)malloc(100);
        unfiltered_value = (char*)malloc(100);
        char temp_field[strlen(field[i])];
        strcpy(temp_field,field[i]);
        strcpy(unfiltered_key,strtok(temp_field,":"));
        char* tok_value=strtok(NULL,":");
        strcat(unfiltered_value,tok_value);
        tok_value=strtok(NULL,":");
        while((tok_value!=NULL)){
            strcat(unfiltered_value,":");
            strcat(unfiltered_value,tok_value);
            tok_value=strtok(NULL,":");
        }
        char* filtered_key=strtok(unfiltered_key," ");
        unfiltered_value=unfiltered_value+1;
        if(strcmp(filtered_key,"autore") == 0){
            book->autore[n_autori] = unfiltered_value;
            n_autori++;
        }else if(strcmp(filtered_key,"titolo") == 0){
            book->titolo = unfiltered_value;
        }else if(strcmp(filtered_key,"editore") == 0){
            book->editore = unfiltered_value;
        }else if(strcmp(filtered_key,"nota") == 0){
            book->nota = unfiltered_value;
        }else if(strcmp(filtered_key,"collocazione") == 0){
            book->collocazione = unfiltered_value;
        }else if(strcmp(filtered_key,"luogo_pubblicazione") == 0){
            book->luogo_pubblicazione = unfiltered_value;
        }else if(strcmp(filtered_key,"anno") == 0){
            book->anno = atoi(unfiltered_value);
        }else if(strcmp(filtered_key,"prestito") == 0){
            book->prestito = unfiltered_value;
        }else if(strcmp(filtered_key,"descrizione_fisica") == 0){
            book->descrizione_fisica = unfiltered_value;
        }
    }
    return book;
}

void* worker(void* args){
    Queue_t* q=((arg_t*)args)->q;
	int* fdMax=((arg_t*)args)->fdMax;
	fd_set* clients=((arg_t*)args)->clients;

	pthread_mutex_t* m=((arg_t*)args)->mutex;
	char buf[N];
	while(1){
		int* fd=(int*)pop(q);
		
		free(fd);
	}
	return NULL;
}
