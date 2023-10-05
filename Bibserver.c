#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>

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


int main(int argc, char* argv[]){
    if(argc!=4){
        printf("Not the right amount of args\n");
        _exit(EXIT_FAILURE);
    }
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int w = atoi(argv[3]);
    FILE* fin = fopen(file_name,"r");
    size_t size=400;
    char* riga;
    int nchar_readed;
    char* field;
    char* key;
    char* value;
    Book book;
    book.prestito=NULL;
    if(fin){
        while((nchar_readed=getline(&riga,&size,fin)) && !feof(fin)){
            if(nchar_readed!=1)
            {
                field=strtok(riga,";");
                int n_autori=0;
                do{
                    key= strtok(field,":");
                    value = strtok(NULL,":");
                    if(strcmp(key,"autore") == 0){
                        book.autore[n_autori] = value;
                        n_autori++;
                    }else if(strcmp(key,"titolo") == 0){
                        book.titolo = value;
                    }else if(strcmp(key,"editore") == 0){
                        book.editore = value;
                    }else if(strcmp(key,"nota") == 0){
                        book.nota = value;
                    }else if(strcmp(key,"collocazione") == 0){
                        book.collocazione = value;
                    }else if(strcmp(key,"luogo_pubblicazione") == 0){
                        book.luogo_pubblicazione = value;
                    }else if(strcmp(key,"anno") == 0){
                        book.anno = atoi(value);
                    }else if(strcmp(key,"prestito") == 0){
                        book.prestito = value;
                    }else if(strcmp(key,"descrizione_fisica") == 0){
                        book.descrizione_fisica = value;
                    }
                printf("%s\n",field);
                }while((field=strtok(NULL,";"))!=NULL);
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