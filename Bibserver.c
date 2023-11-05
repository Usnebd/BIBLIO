#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#define SIZE 400
struct
{
    char* titolo;
    char* editore;
    int anno;
    char* nota;
    char* collocazione;
    char* luogo_pubblicazione;
    char* descrizione_fisica;
    char* prestito;
    char* autore[];
}typedef Book;

int countAttributes(char* str);
Book* get_record(char* riga, Book* book);

int main(int argc, char* argv[]){
    if(argc!=4){
        printf("Not the right amount of args\n");
        _exit(EXIT_FAILURE);
    }
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int w = atoi(argv[3]);
    size_t size=SIZE;
    char* riga=(char*)malloc(sizeof(char)*SIZE);
    int nchar_readed;
    FILE* fin = fopen(file_name,"r");
    if(fin){
        while((nchar_readed=getline(&riga,&size,fin)) && !feof(fin)){
            if(nchar_readed!=1)
            {   
                char temp_riga[strlen(riga)];
                strcpy(temp_riga,riga);
                Book* book = (Book*)malloc(sizeof(Book));
                get_record(riga,book);
            }
        }
        if(ferror(fin)){
            perror("Errore durante la lettura");
        }

        fclose(fin);
    }else{
        perror("Errore apertura file");
    }
    _exit(EXIT_SUCCESS);
}



int countAttributes(char* riga){
    int count=0;
    strtok(riga,";");
    while(strtok(NULL,";")!=NULL){
        count++;
    }
    return count;
}

Book* get_record(char* riga, Book* book){
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
            printf("%s\n",book->titolo);
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