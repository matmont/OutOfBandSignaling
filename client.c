/*
    Matteo Montalbetti
    MAT. 559881
    Corso B
    "OUT-OF-BAND SIGNALING"
*/

/*
    Le seguenti define sono state inserite per
    ovviare ad errori/warning durante la compilazione;
*/
#define _POSIX_C_SOURCE 200809L
#define UNIX_PATH_MAX 108
#define FROM_SECRET_TO_NANO 1000000

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<time.h>
#include<errno.h>
#include<sys/socket.h>
#include<unistd.h>
#include<time.h>
#include<arpa/inet.h>
#include<string.h>
#include<signal.h>
#include<bits/sigaction.h>

#include"aux-functions.h"
#include"err-managements.h"

/*  RICHIESTA DEL PROGETTO: 
    All'avvio, il client genera il suo secret, che e costituito da un numero fra 1 e 3000, e il suo ID unico, che e un
    intero a 64 bit; entrambi devono essere generati pseudo-casualmente (si ricordi di inizializzare il generatore
    casuale in maniera appropriata). Un client riceve sulla riga di comando tre valori interi, p, k e w, con 1 ≤ p <
    k e w > 3p. Il secondo dovra essere lo stesso passato al supervisor, che controlla quanti server sono attivi sul
    sistema.
    All'inizio, il client deve stampare sul proprio stdout un messaggio nel formato “CLIENT id SECRET secret”,
    in cui id e rappresentato da un numero esadecimale (senza prefissi particolari), e secret da un numero
    decimale. Il client dovra quindi scegliere casualmente p interi distinti compresi fra 1 e k, e si colleghera
    (tramite socket, vedi avanti) ai p server corrispondenti agli interi scelti. Iniziera un ciclo in cui, ad ogni
    iterazione, scegliera casualmente un server fra i p a cui e collegato, gli inviera sulla socket corrispondente un
    messaggio contenente il proprio ID unico (il formato sara un buffer di 8 byte contenente l'ID in network byte
    order), e attendera secret millisecondi prima di ripetere (si puo usare la system call nanosleep(2) per
    l'attesa). Dovranno essere inviati complessivamente w messaggi (in totale, non per ciascun server). Una volta
    completato il proprio compito, il client termina, dopo aver stampato un messaggio “CLIENT id DONE”
*/


int main(int argc, char* argv[]) {
    
    /*
        Dichiarazione di alcune variabili.

        p --> 'p' è un parametro che 'client' prende in input. Rappresenta il numero di server
              a cui il client dovrà collegarsi, scegliendoli tra i k disponibili;
        k --> 'k' è un parametro che 'client' prende in input. Rappresenta il numero di server che
              sono stati lanciati dal supervisor (per questo motivo dovrà essere uguale al 'k' del
              supervisor). Tra questi 'k' il client ne sceglierà 'p' a cui inviare il proprio ID;
        w --> 'w' è un parametro che 'client' prende in input. Rappresenta il numero totale di 
              messaggi che il client, divendoli tra i vari 'p' server, invierà;
        id_one, id_two --> il progetto richiede che l'ID sia un intero di 64 bit. Per dichiarare un
                           intero di questo tipo si potrebbe utilizzare il tipo 'uint64_t'. Il problema,
                           però, è che questo ID dovrà essere convertito in 'network byte order' tramite
                           la funzione 'htonl'. Questa funzione però opera su 'uint32_t'; ciò che ho 
                           deciso di fare, allora, è quello di avere due "pezzi" di ID, entrambi 'uint32_t',
                           che combinati mi porteranno ad avere un 'uint64_t' (64 bit).
        final_id --> variabile di tipo 'uint64_t' nel quale viene salvata la combinazione dei due "pezzi"
                     di ID;
        choices --> è un array di interi. Mi serve per tenere traccia degli indici dei 'p' server che sono stati scelti
                    dal client. ("...Il client dovrà quindi scegliere casualmente p interi distinti compresi fra 1 e k...")
        fd_sockets --> è un array di interi. Mi serve per tenere traccia dei descrittori dei file delle varie socket a cui
                       il client si collega.
        socket_name --> è una stringa. Rappresenta l'indirizzo della socket al quale il client dovrà collegarsi.
        buffer --> è un buffer. Viene utilizzato come buffer per la scrittura del messaggio. Ci si scrive l'ID prima
                   di inviarlo al server.
        
    */

    //Input
    int p, k, w;
    //Secret e ID
    int secret;
    
    uint32_t id_one;
    uint32_t id_two;
    uint64_t final_id;
    //Networking
    int* choices;
    int* fd_sockets;
    char* socket_name;
    //Buffer
    char buffer[8] = {0};   
    
    /*
        Nelle seguenti 4 righe di codice ho inizializzato il generatore pseudo-casuale; in parole povere,
        ho scelto il seed da passare ad 'srand'. 

        Perchè è importante questo passaggio? 
        Solitamente, un generatore viene inizializzato passando ad 'srand' il valore 'time(NULL)'.
        Nello script 'test.sh', però, si lanciano due programmi client (praticamente) simultaneamente:
        avrebbero quindi avuto stesso valore di 'time(NULL)' e per questo avrebbero generato i soliti
        numeri casuali, nel solito ordine, finendo per avere ID e SECRET identici. Questo non va bene!
        Ho deciso quindi di scegliere un 'seed' che non dipendesse solamente dal tempo, ma anche dall'
        ID del processo.

        Ho comunque preferito una struttura di tipo 'timeval' rispetto al semplice 'time(NULL)' per la
        maggiore precisione.
    */
    struct timeval t;
    gettimeofday(&t, NULL);
    unsigned long seed = t.tv_usec * t.tv_sec * getpid();
    srand(seed);

    /*
        Gestione dell'input con il quale il programma è stato lanciato. Sostanzialmente quello che
        succede è la conversione dell'input (che avrà formato 'char*') in intero e assegnazione
        di questo valore alle variabili 'p', 'k' e 'w' di cui abbiamo già parlato.

        E' presente anche un'altra funzione, ovvero 'check_client_inputs'. Questa è una
        funzione ausiliaria che ho deciso di creare per mantenere il codice del client più pulito. 
        Ciò che fa è controllare e notificare i vari errori che potrebbero esserci nell'input inserito; 
        nel caso dovessero esserci problemi, il programma viene immediatamente terminato tramite una 'exit(EXIT_FAILURE)' 

        Per ulteriori chiarimenti sull'implementazione della funzione, consultare i commenti
        contenuti nel file 'aux-functions.c';
    */
    check_client_inputs(argc, argv);

    p = strtol(argv[1], NULL, 10);
    k = strtol(argv[2], NULL, 10);
    w = strtol(argv[3], NULL, 10);
    
    /*
        In questa fase vengono generati in maniera pseudo-casuale il SECRET e i due
        "pezzi" di ID, i quali vengono poi combinati nell'unico 'final_id' di 64 bit.

        La formula di ricombinazione è stata trovata in Internet;
    */
    secret = rand() % 3000;
    id_one = rand();
    id_two = rand();
    final_id = (uint64_t) id_one << 32 | id_two;

    //Messaggio di notifica (come richiesto dal progetto)
    printf("CLIENT %x SECRET %d\n", (int) final_id, secret);
    /*
        In alcuni file del progetto si potrà trovare più volte il comando 'fflush(stdout)'.
        Ho inserito questo comando per far sì che lo stdout di 'supervisor' non sia bufferizzato. 
        Ho fatto questo perchè al momento dei test, lo stdout dei programmi viene indirizzato 
        all'interno di alcuni file di testo; se non mettessi questo comando, il file di testo 
        verrebbe scritto in maniera molto confusionaria, rendendo difficile la comprensione e 
        soprattutto l'interpretazione fatta da 'misura.sh' (vedere il file per chiarimenti).
    */
    fflush(stdout);
    
    /*
        Nelle successive righe di codice, viene eseguito il primo ciclo: la scelta dei 'p' server
        a cui il client dovrà collegarsi.

        Ho deciso di dividere il ciclo della scelta rispetto al ciclo delle connessioni per avere
        più pulizia e modularità nel codice.

        Per prima cosa alloco 'choices' come un array di 'p' interi.

        Una volta fatto ciò proseguo alla scelta di p numeri casuali tra 1 e k. Chiaramente,
        i numeri scelti dovranno essere differenti l'uno dall'altro (motivo per cui è presente quel while; un'
        altra soluzione prevede l'uso del "do-while").

        Salvo infine l'indice scelto all'interno dell'array 'choices'.
    */
    choices = _calloc(p, sizeof(int));
    
    //p numeri casuali  
    for (int i = 0; i < p; i++)
    {
        int random = (rand() % (k)) + 1;  //lo genero tra 0 e k-1 e ci aggiungo 1 così che sarà tra 1 e k
        while(member(random, choices, p)){
            random = (rand() % (k)) + 1;
        }
        *(choices+i) = random;
    }

    /*  
        Il seguente ciclo implementa la connessione da parte del client ai 'p' server,
        tramite le socket di indirizzi 'OOB-server-[i]'.

        E' stato anche qui allocato l'array 'fd_sockets' così da poter salvare ogni
        volta il descrittore di ogni socket.
    */

    fd_sockets = _calloc(p, sizeof(int));

    for (int i = 0; i < p; i++)
    {
        //Allocazione di 'socket_name'
        socket_name = _calloc(UNIX_PATH_MAX, sizeof(char));
        sprintf(socket_name, "OOB-server-%d", *(choices+i));

        struct sockaddr_un sa;
        strncpy(sa.sun_path, socket_name, UNIX_PATH_MAX);
        sa.sun_family = AF_UNIX;

        fd_sockets[i] = _socket();
        _connect(fd_sockets[i], sa);
        //Free di 'socket_name'
        free(socket_name);
    }

    /*
        Preparazione del messaggio da inviare, eseguo la solita combinazione fatta in 
        precedenza. Se prima combinavo 'id_one' e 'id_two', stavolta vengono invece
        combinati 'htonl(id_one)' e 'htonl(id_two)'.

        Questo perchè l'ID dev'essere inviato in 'network byte order' (come richiesto dal 
        progetto).
    */
    sprintf(buffer, "%x", (int) ((uint64_t) htonl(id_one) << 32 | htonl(id_two)));

    /*
        Il seguente ciclo si occupa dell'invio dei w messaggi ai vari server.
        NOTA BENE --> vengono inviati w messagi in totale; arriveranno i messaggi
        a quei server che vengono scelti tramite la generazione pseudo-casuale.

        Prima di iniziare il ciclo, viene calcolato il tempo (basato sul SECRET) che
        il client dovrà attendere prima inviare un messaggio.
    */
    struct timespec sleep_time = {secret/1000, secret%1000 * 1000000L};
    for (int i = 0; i < w; i++)
    {
        //Scelto un server tra i 'p' a cui sono connesso
        int random = (rand() % p);

        /*  [ripreso dal commento in 'server', riga 214]
            Per la lettura (ed anche la scrittura) del messaggio ho deciso di utilizzare una tecnica 
            per minimizzare lo spreco di risorse e il rischio di leak.

            Piuttosto che inviare un unico messaggio, si puo' far si' che il mittente invii due messaggi:
            il primo rappresenta solamente la 'size' di cui il destinatario dovra' allocare il buffer
            di ricezione per ricevere senza avere problemi (e senza sprechi) il messaggio vero e proprio.
            Il secondo messaggio sara' effettivmente il messaggio originario. 

            Questo e' possibile perche' la dimensione del buffer di ricezione per il primo messaggio e' 
            nota! La 'size' e' un valore intero, e mi bastera' quindi leggere usando un 'int' come buffer di
            ricezione;
        */
        int size = strlen(buffer) + 1;

        write(fd_sockets[random], &size, sizeof(int));
        write(fd_sockets[random], buffer, size);

        //Attesa prima del prossimo invio
        nanosleep(&sleep_time, NULL);
    
    }
    
    /*
        Infine, alla conclusione del client, viene stampato un messaggio di uscite (come
        richiesto dal progett) e soprattutto viene liberata in maniera più corretta possibile
        la memoria allocata.

        Per prima cosa vengono chiuse tutte le socket a cui il client si era collegato (così
        da permettere la terminazione delle lettura da parte dei server).

        Successivamente si stampa il messaggio di uscita; infine, viene deallocata la memoria
        precedentemente allocata. 
    */
    for (int i = 0; i < p; i++)
    {
        close(fd_sockets[i]);
    }

    printf("CLIENT %x DONE\n", (int) final_id);
    /*
        In alcuni file del progetto si potrà trovare più volte il comando 'fflush(stdout)'.
        Ho inserito questo comando per far sì che lo stdout di 'supervisor' non sia bufferizzato. 
        Ho fatto questo perchè al momento dei test, lo stdout dei programmi viene indirizzato 
        all'interno di alcuni file di testo; se non mettessi questo comando, il file di testo 
        verrebbe scritto in maniera molto confusionaria, rendendo difficile la comprensione e 
        soprattutto l'interpretazione fatta da 'misura.sh' (vedere il file per chiarimenti).
    */
    fflush(stdout);
    
    free(choices);
    free(fd_sockets);

    fflush(NULL);

    return 0;

}
