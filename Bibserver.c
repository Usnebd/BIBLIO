#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>

#define BUFF 1000

struct file_record
{
    char* autore[];
    char* titolo;
    char* editore;
    int anno;
    char* nota;
    char* collocazione;
    char* luogo_pubblicazione;
    char* descrizione_fisica;
    char* prestito;
};


int main(int argc, char* argv[]){
    char* name_bib = argv[1];
    char* file_name = argv[2];
    int w = atoi(argv[3]);
    File* fin = fopen(file_name,"r");
    char testo[BUFF];
    int len=0;
    if(fin){
        while(!feof(fin) && len<BUFF-1 && fgets(&testo[len],BUFF-len,fin)){
            len=strlen(testo);
        }
        if(ferror(fin)){
            perror("Errore durante la lettura");
        }else{
            if(!feof(fin)){
                printf("Buffer pieno.\n");
            }
        }
        fclose(fin);
    }else{
        perror("Errore apertura file");
    }
    _exit(EXIT_SUCCESS);
}