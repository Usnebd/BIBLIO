#!/bin/bash

totalsum=0
option=""
upcaseOption=""
# Trova l'opzione
for arg in "$@"; do
    case "$arg" in
        "--query")
            option="$arg"
            upcaseOption="QUERY"
            ;;
        "--loan")
            option="$arg"
            upcaseOption="LOAN"
            ;;
        *)
            ;;
    esac
done

# Inizializza la somma totale
totalsum=0
echo
# Itera sui file
for file in "$@"; do
    sum=0
    # Ignora l'opzione e i file relativi                      #verifica se la variabile file non corrisponde all'espressione regolare ^--
    if [[ "$file" != "$option" && ! "$file" =~ ^-- ]]; then   # =~ è utilizzato per confrontare una stringa con un'espressione regolare
        while IFS= read -r line || [ -n "$line" ]; do         #IFS= Imposta il separatore di campo interno a nulla
            if [[ "$line" == "$upcaseOption"* ]]; then        #read -r line: Legge una riga dal file e assegna il valore alla variabile line
                number=${line##* }                            #Verifica se la variabile line inizia con il valore di upcaseOption
                sum=$((sum + number))                         #${line##* }: Questo estrae la parte della stringa di line dopo l'ultimo spazio.
            fi
        done < "$file"                                        #specifica il file da cui leggere,  < è l'operatore di reindirizzamento dell'input

        totalsum=$((totalsum + sum))
        echo "$file $sum"
    fi
done

if [[ "$totalsum" -gt 0 ]]; then                              #se totalsum è maggiore di 0
    echo "$upcaseOption $totalsum"
fi
