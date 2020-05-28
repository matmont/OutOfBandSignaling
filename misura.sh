#!/bin/bash

# # # # # # # # # # # # # # # # 
#    Matteo Montalbetti       #
#    MAT. 559881              #
#    Corso B                  #
#    "OUT-OF-BAND SIGNALING"  #
# # # # # # # # # # # # # # # #

# Si dovra realizzare uno script bash di nome misura che, ricevuti come argomenti i nomi di un insieme di file
# contenenti l'output di supervisor, server e client, ne legga e analizzi i contenuti, e stampi delle statistiche su
# quanti secret sono stati correttamente stimati dal supervisor (intendendo per stima corretta un secret stimato
# con errore entro 25 unita rispetto al valore del secret vero), e sull'errore medio di stima. 

# Lo script 'misura.sh' prende in input tre argomenti:
# - il numero di client --> serve per gestire in maniera corretta il file di testo contenente l'output di 'supervisor';
# - file del client --> file sul quale e' stato rediretto lo stdout e stderr del client;
# - file del supervisor --> file sul quale e' stato rediretto lo stdout e stderr di supervisor e server;
CLIENTS_NUMBER=$1;
CLIENT_FILE=$2;
SUPER_FILE=$3;

# Dichiaro ed inizializzo due variabili che tengono il conto
# delle stime corrette e di quelle errate.
CORRECT_EST=0
WRONG_EST=0
# Dichiaro ed inizializzo una variabile che tiene il conto dell'
# errore totale commesso (in termini quantitativi) per tutte le stime
TOTAL_ERROR=0

# Gestisco e modifico i file di testo contenenti lo stdout e stderr dei programmi.
# 
# La prima riga consiste nella lavorazione del file 'supervisor.txt': anzitutto prendiamo 
# dal file 'supervisor' le ultime $CLIENTS_NUMBER + 1 righe (comando 'tail'); una volta fatto
# cio' eliminiamo la riga contenente il messaggio di uscita del supervisor. Il tutto viene inserito
# all'interno di un nuovo file chiamato 'rev_supervisor.txt'. Cosi' facendo in questo file 
# saranno contenute le sole ed ultime 20 righe della tabella definitiva.
# 
# La seconda riga riguarda invece la lavorazione del file 'client.txt': vengono semplicemente eliminate
# tutte le righe contenenti la parola 'DONE' (messaggio di uscita del client). Cosi facendo
# rimangono solo le 20 righe dei messaggi di entrata, contenenti l'ID e il SECRET, le due informazioni
# di cui abbiamo bisogno. (Le modifiche sono fatte nel file stesso tramite l'opzione '-i')
tail -$(($CLIENTS_NUMBER + 1)) $SUPER_FILE | sed '/SUPERVISOR EXITING/d' > rev_supervisor.txt
sed -i '/DONE/d' $CLIENT_FILE

# Ciclo sul file 'client.txt', analizzandolo riga per riga;
# per ogni riga del file 'client.txt', e quindi per ogni client
# cerco nel file 'rev_supervisor.txt' la riga relativa al medesimo
# client (tramite l'ID). Una volta trovata la riga corretta nel
# file 'rev_supervisor.txt' procedo al confronto tra le due stime,
# aggiornando in base all'esito di questo le variabili 'CORRECT_EST' o 'WRONG_EST'
while IFS= read -r line
do 
    # Salvo in due variabili le informazioni relative al client
    # della riga corrente.
    # Tramite il comando 'cut' riesco a tokenizzare e recuperare
    # un pezzo della riga; recupero in questo modo l'ID e il SECRET.
    CLIENT_ID=$(echo $line | cut -d ' ' -f 2)
    CLIENT_SECRET=$(echo $line | cut -d ' ' -f 4)
    # Ciclo while per la ricerca, nel file 'rev_supervisor.txt',
    # della riga riguardante il client corrispondente;
    while IFS= read -r line 
    do
        # Controllo se la riga corrente di 'rev_supervisor.txt'
        # riferisce al medesimo client cui riferisce la riga corrente
        # di 'client.txt'.
        SUPER_ID=$(echo $line | cut -d ' ' -f 5)
        if [ "$SUPER_ID" = "$CLIENT_ID" ]; then
            SUPER_ESTIMATE=$(echo $line | cut -d ' ' -f 3)
            # Stampo un messaggio a fine informativo
            echo "ID $CLIENT_ID with SECRET $CLIENT_SECRET --> SUPERVISOR best estimate $SUPER_ESTIMATE"
            # Calcolo ora due variabili che mi faranno da "edge" per il
            # testing della stima del secret. Una stima e' considerata corretta se
            # differisce dal secret reale AL PIU per 25 unita'. Calcolo allora 
            # 'secret - 25' e 'secret + 25' per poi verificare se la stima rientra in questo
            # intervallo oppure no.
            LOW_RANGE=$(( CLIENT_SECRET-25 ))
            HIGH_RANGE=$(( CLIENT_SECRET +25 ))
            # Confronto e incremento le variabili in base all'esito di questo;
            if [ $SUPER_ESTIMATE -gt $LOW_RANGE -a $SUPER_ESTIMATE -lt $HIGH_RANGE ] ; then
                (( CORRECT_EST++ ))
            else 
                (( WRONG_EST++ ))
                echo "***errore***"
            fi
            # Per il calcolo dell'errore medio ho bisogno di tenere traccia 
            # della quantita' di cui la stima differisce dal reale secret.
            # Ho reputato necessario un controllo tra la stima e il 
            # secret per impostare la differenza in modo che non vengano 
            # risultati negativi
            if [ $SUPER_ESTIMATE -gt $CLIENT_SECRET ] ; then
                DIFFERENCE=$(( SUPER_ESTIMATE-CLIENT_SECRET ))
                TOTAL_ERROR=$(( TOTAL_ERROR+DIFFERENCE ))
            else 
                DIFFERENCE=$(( CLIENT_SECRET-SUPER_ESTIMATE ))
                TOTAL_ERROR=$(( TOTAL_ERROR+DIFFERENCE ))
            fi

        fi

    done < rev_supervisor.txt

done < $CLIENT_FILE

# Stampo i risultati della misurazione
echo "Stime corrette -> $CORRECT_EST"
echo "Stime errate -> $WRONG_EST"

# Calcolo e stampo l'errore medio; per avere un risultato che
# sia decimale ho utilizzato il tool 'bc'.
AVG_ERROR=$(echo "scale=2; $TOTAL_ERROR/$CLIENTS_NUMBER" | bc)

# Ho notato che utilizzando il tool 'bc', un risultato
# minore di 1, quindi della forma 0.X, viene salvato nella
# variabile 'AVG_ERROR' omettendo lo 0 prima del punto (quindi,
# verrebbe salvato solatemente .X).
# Controllo allora se il risultato dovesse essere minore di 1
# per aggiungere lo 0 omesso in fase di stampa.
if [ $(( TOTAL_ERROR/CLIENTS_NUMBER )) -lt 1 ] ; then
    echo "Errore medio -> 0$AVG_ERROR"
else
    echo "Errore medio -> $AVG_ERROR"
fi

########################################################

# Sebbene non richiesto dal progetto, ho preferito inserire
# anche alla fine di questo script le operazioni di pulizia
# della directory, cosi' da non dover eseguire 'make clean' 
# dopo ogni test.

rm -f OOB-server-* *.o *.txt client supervisor server

####################################################
