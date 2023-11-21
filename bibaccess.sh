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
    # Ignora l'opzione e i file relativi
    if [[ "$file" != "$option" && ! "$file" =~ ^-- ]]; then
        while IFS= read -r line || [ -n "$line" ]; do
            if [[ "$line" == "$upcaseOption"* ]]; then
                number=${line##* }
                sum=$((sum + number))
            fi
        done < "$file"

        totalsum=$((totalsum + sum))
        echo "$file $sum"
    fi
done

if [[ "$totalsum" -gt 0 ]]; then
    echo "$upcaseOption $totalsum"
fi
