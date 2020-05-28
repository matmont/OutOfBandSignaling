# !/bin/bash

# # # # # # # # # # # # # # # # 
#    Matteo Montalbetti       #
#    MAT. 559881              #
#    Corso B                  #
#    "OUT-OF-BAND SIGNALING"  #
# # # # # # # # # # # # # # # #

#" ... Quest'ultimo deve eseguire un ciclo completo di test, lanciando il supervisor con 8 server,
# e dopo un'attesa di 2 secondi dovra lanciare un totale di 20 client, ciascuno con parametri 5 8 20.
# I client andranno lanciati a coppie, e con un attesa di 1 secondo fra ogni coppia.
# Dopo aver lanciato l'ultima coppia, il test dovra attendere 60 secondi, inviando nel frattempo un SIGINT al
# supervisor ogni 10 secondi; al termine dovra inviare un doppio SIGINT, e lanciare lo script misura sui
# risultati raccolti (tramite redirezione dello stdout dei vari componenti su appositi file) ... "

# Stampa per notificare l'imminente esecuzione dell'eseguibile 'supervisor';
# si utilizza il comando 'echo'.
echo "Starting supervisor"

# Esecuzione dell'eseguibile 'supervisor' con input 8; verranno quindi lanciati 8 server.
# Attraverso la combinazione di comandi '&> file &'' redirigo lo stdout E LO STDERR dell'eseguibile
# nel file 'supervisor.txt' che verra' appositamente creato.
./supervisor 8 &> supervisor.txt &
sleep 2

# Lancio i 20 client, stampando un messaggio di notifica ogni 2 client avviati.
for ((i = 0; i< 10; i++)); do
    ./client 5 8 20 &>> client.txt &
    ./client 5 8 20 &>> client.txt &
    echo "Launched 2 clients"
    sleep 1
done

# Lancio 6 SIGINT una ogni 10 secondi, stampando un messaggio di notifica per ogni SIGINT.
for ((i = 0; i < 6; i++)); do
    pkill -INT supervisor
    echo "Launched INT signal"
    sleep 10
done

# Lancio il doppio segnale SIGINT.
echo "Double INT signal"
pkill -INT supervisor
pkill -INT supervisor


echo "Launching misura.sh"

# Lancio lo script che si occupa della misurazione;
# 'misura.sh' prende come parametro di input il numero di client
# sui quali sono state fatte le stime.
bash misura.sh 20 client.txt supervisor.txt