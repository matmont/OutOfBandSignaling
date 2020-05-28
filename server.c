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
#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 200809L
#define UNIX_PATH_MAX 108

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<sys/socket.h>
#include<string.h>
#include<time.h>
#include<arpa/inet.h>
#include<signal.h>
#include<bits/sigaction.h>

/*
    Ho deciso di creare tre file esterni per raccogliere alcune 
    funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione. La spiegazione di ognuno e' 
    commentata nei rispettivi file '.c'e '.h';
*/

#include"err-managements.h"

/*RICHIESTA DEL PROGETTO:
Un server (che viene lanciato dal supervisor) per prima cosa apre una socket nel dominio AF_UNIX (si usi
come indirizzo la stringa “OOB-server-i”, in cui i e il suo numero progressivo fra 1 e k). Procede quindi ad
attendere connessioni da parte dei client, che come detto possono arrivare in qualunque momento. Per ogni
connessione, il server dovra osservare il momento di arrivo dei messaggi da parte del client, e in particolare
il tempo trascorso fra messaggi consecutivi. Per come e strutturato il client, il tempo fra due messaggi
consecutivi allo stesso server sara un multiplo del suo secret. Compito del server e di stimare il secret di ogni
client, e – nel momento in cui la connessione viene chiusa da quel client – informare il supervisor di qual'e la
sua migliore stima, inviando un messaggio che contenga l'ID del client e la sua stima del secret.
All'avvio il server deve stampare sul suo stdout un messaggio nel formato “SERVER i ACTIVE”. Per ogni
nuova connessione ricevuta, deve stampare un messaggio “SERVER i CONNECT FROM CLIENT”. Per
ogni messaggio ricevuto sulla socket, deve stampare su stdout un messaggio “SERVER i INCOMING
FROM id @ t”, in cui id e l'ID del client che ha inviato il messaggio, e t e il tempo di arrivo (espresso come
un numero, con unita di misura a discrezione). Per ogni chiusura della socket, deve stampare su stdout un
messaggio “SERVER i CLOSING id ESTIMATE s”, in cui s e il valore stimato da questo client per il secret
del client id.
Al momento della chiusura, il server deve comunicare al supervisor la sua migliore stima per il valore del
secret di id (la stessa stampata nel messaggio). La comunicazione fra server e supervisor e realizzata tramite
pipe anonime come descritto sotto. 
*/

/*
    Cosi' come per il supervisor, anche per il server ho deciso di dichiarare alcune variabili
    globalmente, cosi' da poterle raggiungere dalle funzioni esterne al main.

    id --> rappresenta l'"id" del server; piu che id, e' in realta' semplicemente il numero
           progressivo assegnato al server del supervisor (compreso quindi tra 1 e k);
    pfd --> rappresenta il descrittore del file della pipe da utilizzare in scrittura
            per comunicare con il supervisor;
    
    ... [continua nel successivo]


*/
int id, pfd;

/*
    Ho poi dichiarato alcune variabili utili alla gestione dei thread:

    tid --> rappresenta un array che tiene traccia dei vari id dei thread creati; serve
            per poter poi fare la join correttamente.
    finished_threads --> ho deciso di utilizzare un array che mi tenga traccia di quali thread hanno
                         concluso la loro esecuzione (tramite la 'pthread_exit'), in modo tale da poterli
                         terminare definitivamente e correttamente attraverso la join.

                         "il thread avente come tid tid[j] e' terminato se finished_threads[j] == 1"

    to_thread --> e' una struct che ho creato per passare al thread worker due dati: l'indice i del thread, in modo
                  tale da poter settare finished_threads[i], e il descrittore della socket del client, in modo che 
                  il thread possa ricevere dati da esso;

*/
pthread_t tid[SOMAXCONN];
int finished_threads[SOMAXCONN];

struct to_thread {
    int index;
    int fd_c;
};

/*
    'manage_function' e' la funzione chiamata in causa nel momento in cui il programma
    'server' riceve un segnale di tipo SIGTERM. 

    Il problema stava nel fatto che i server non avessero una condizione di terminazione. Dovevano
    restare attivi fino a quando il supervisor fosse in esecuzione. Chiaramente pero' una volta che
    il supervisor viene terminato e' necessario terminare esplicitamente e correttamente i vari 
    processi figli, ovvero i server. Questo avviene grazie al lancio di un segnale SIGTERM da parte
    del supervisor a tutti i figli, i quali "reagiscono" a quel segnale seguendo questo gestore.

    Cio' che 'manage_function' fa e' la chiusura del descrittore della pipe, e la corretta
    join di tutti i thread terminati.
*/
static void manage_function(int signum) {
    for (int j = 0; j < SOMAXCONN; j++)
    {   
        //Per tutti i thread controllo se ce ne è qualcuno che ha finito 
        //e faccio la corretta terminazione
        if(finished_threads[j] == 1) {
            _join_thread(tid[j], NULL);
        }
    } 
    close(pfd);
    fflush(NULL);
    exit(0);
}



/*
    La seguente funzione 'worker' determina il comportamento che i vari thread lanciati dal
    server devono avere (definisce la comunicazione tra server e client);

    Sebbene per la supervisor - server io abbia deciso di utilizzare il meccanismo
    della select, per la comunicazione server - client ho deciso di adottare un'architettura
    multithreading. 
    Questo vuol dire che il processo iniziale del server fungera' da thread DISPATCHER, il quale
    lancera' un thread WORKER per ogni connessione con un nuovo client.

    Quello che sostanzialmente fa questa funzione e' molto semplice:
        - si occupa della lettura nella socket
        - si occupa della misurazione e conseguente stima
        - si occupa della comunicazione al supervisor della stima

    Vediamo meglio i vari punti al suo interno;
*/
void* worker(void* arg) {
    /*
        Dichiarazione di alcune variabili:
        
        message --> buffer per la lettura. Qui verra' salvato il messaggio letto dalla socket;
        to_super --> buffer per la scrittura. Useremo 'to_super' come buffer per la comunicazione
                     al supervisor della stima;
        args --> rappresenta l'argomento passato in input alla funzione. Passeremo al thread worker la
                 struct 'to_thread' di cui abbiamo parlato precedentemente, e procederemo poi al recupero
                 ('socket_client' e 'thread_index') dei due dati contenuti in essa;
        first --> utilizzo un flag per controllare se il thread deve ancora ricevere il primo messaggio
                  oppure ne ha gia' ricevuti;
        estimate --> variabile nel quale verra' salvata la stima fatta dal thread;
        client_id --> e' un buffer di 8 byte, nel quale salvo l'ID del client con cui sto comunicando (il
                      client inviera' sulla socket proprio il suo ID);
        best_sub --> l'aggiornamento della stima dipende dalla grandezza della piu' recente rispetto a quella
                     gia' presente in 'estimate'. In generale, vince la stima minore! (le stime
                     saranno o il secret, o un multiplo di esso) 
                     Aggiorno (se necessario) questa variabile ad ogni ciclo per avere la migliore stima; una
                     volta uscito dal ciclo while, assegnero' poi questo valore alla variabile 'estimate';

        ... [continua nel successivo]

    */
    char* message;
    char* to_super;

    struct to_thread* args;
    args = ((struct to_thread*) arg);
    int socket_client = args->fd_c;
    int thread_index = args->index;

    int first = 1;

    int estimate = 0;

    char client_id[8] = {0};
    
    long best_sub = 0;

    /*
        Le operazioni di stima sono basate sul tempo trascorso tra l'arrivo di un messaggio ed il
        successivo, questo perche' il client dopo aver inviato un certo messaggio esegue una sleep di 'secret'
        millisecondi.

        Ho deciso di utilizzare per la misurazione del tempo il metodo '_gettimeofday' (che ho ridefinito per la
        gestione degli errori in 'err-managements.c') il quale salva il tempo attuale in variabili di tipo 'struct timeval'.
        Queste struct sono formate da due dati, i secondi e i microsecondi, per una precisione piu' accurata.

        previous_message --> 'struct timeval' che tiene traccia del momento di arrivo del messaggio precedente;
        new_message --> 'struct timeval' che tiene traccia del momento di arrivo del messaggio appena ricevuto;
                         la differenza tra questi rappresenta una stima;
        thread_init --> 'struct timeval' che tiene traccia del momento in cui il thread e' stato avviato; questo
                        perche' il progetto richiede di stampare il "tempo trascorso" ad ogni messaggio ricevuto, che
                        io ho interpretato come "il tempo trascorso rispetto all'avvio del thread". 

        Le seguenti due chiamate '_gettimeofday' inizializzano 'previous_message' e 'thread_init'.
    */
    struct timeval previous_message;
    struct timeval new_message;
    struct timeval thread_init;
    
    _gettimeofday(&previous_message);
    _gettimeofday(&thread_init);

    /*
        Ciclo di letture per il thread; si esce dal seguente
        ciclo while solo quando il client chiude la socket 
        con il server, il che vuol dire che non inviera' altri messaggi;
    */
    while(1) {
        /*
            Per la lettura (ed anche la scrittura) del messaggio ho deciso di utilizzare una tecnica 
            per minimizzare lo spreco di risorse e il rischio di leak.

            Piuttosto che inviare un unico messaggio, si puo' far si' che il mittente invii due messaggi:
            il primo rappresenta solamente la 'size' di cui il destinatario dovra' allocare il buffer
            di ricezione per ricevere senza avere problemi (e senza sprechi) il messaggio vero e proprio.
            Il secondo messaggio sara' effettivmente il messaggio originario. 

            Questo e' possibile perche' la dimensione del buffer di ricezione per il primo messaggio e' 
            nota! La 'size' e' un valore intero, e mi bastera' quindi leggere usando un 'int' come buffer di
            ricezione;


            La variabile 'check' e' invece utilizzata per testare il valore tornato dalla funzione '_read'.
            Come si puo' notare dal prefisso della funzione, '_read' e' una ridefinizione della classica 'read'.
            Per ulteriori chiarimenti consultare il file 'err-managements.c';
        */
        int size;
        int check;

        //Procedo alla lettura della 'size';
        check = _read(socket_client, &size, sizeof(int));
        if(check <= 0) break;

        //Alloco 'message' precisamente di 'size' char;
        message = _calloc(size, sizeof(char));
        //Procedo alla lettura dell'ID;
        check = _read(socket_client, message, sizeof(char)*size);
        if(check <= 0) {
            //La socket è stata chiusa lato client;
            //E' necessario deallocare 'message' per evitare leaks;
            free(message);
            break;
        }

        /*
            Se siamo arrivati a questo punto del codice, l'id e' stato correttamente ricevuto.

            Dobbiamo allora procedere alla misurazione della stima; per prima cosa salviamo nella
            struct 'new_message' il tempo di arrivo di questo nuovo messaggio. Una volta fatto cio'
            procediamo al calcolo di due 'long':
                - 'elapsed' --> rappresenta il tempo trascorso tra l'arrivo di questo messaggio e il precedente;
                                sara' quindi la potenziale stima;
                - 'elapsed_from_init' --> rappresenta il tempo trascorso tra l'arrivo di questo messaggio e il tempo di
                                          "start" del thread; serve solo per poterlo stampare nel messaggio di notifica;
            
            Una volta calcolati i due dati, procedo all'aggiornamento di 'previous_message'.

            Fatto cio', devo procedere al controllo della stima appena fatta, rispetto alla migliore stima posseduta ad ora.
            Il controllo viene fatto tramite un semplice confronto: "vince" la stima minore! 
            Ho reputato necessario inserire il flag 'first' per il seguente motivo: quando un server riceve il primo messaggio 
            che il client invia, il tempo trascorso dovrebbe essere 0. Anche se effettivamente 0 non lo e' mai, rimane comunque un
            valore molto piccolo, che vincerebbe qualsiasi confronto durante tutta la comunicazione, diventando cosi' la migliore stima
            fatta. Il problema e' che questo tempo non dipende dal secret del client! 
            Procedo allora al confronto e all'aggiornamento della stima a partire dal SECONDO messaggio ricevuto dal server.
        */
        _gettimeofday(&new_message);

        long elapsed = (new_message.tv_sec-previous_message.tv_sec)*1000000 + new_message.tv_usec-previous_message.tv_usec;        
        long elapsed_from_init = (new_message.tv_sec-thread_init.tv_sec)*1000000 + new_message.tv_usec-thread_init.tv_usec;

        previous_message.tv_sec = new_message.tv_sec;
        previous_message.tv_usec = new_message.tv_usec;
        
        if((!first) && (best_sub == 0 || elapsed < best_sub)) {  
            best_sub = elapsed;
        }
        
        //Nel caso in cui il messaggio appena ricevuto fosse il primo, setto first a 0;
        if(first) first = 0;

        /*
            Abbiamo quindi fatto la stima; ora pero' abbiamo bisogno di prendere
            effettivmente l'informazione contenuta nel messaggio ricevuto, ovvero l'ID
            del client.

            L'ID del client e' scritto in base esadecimale all'interno del buffer 'message'
            ed ha quindi tipo 'char*'.

            Quello che faccio e' convertire il contenuto di 'message' (utilizzando come base 16)
            in 'unsigned long', cosi da poter chiamare su di esso la funzione 'ntohl', che riporta 
            l'ID nel 'byte order' della macchina sul quale gira il server (l'ID e' stato inviato 
            dal client in 'network byte order').

            Una volta fatto cio' inserisco, scrivendolo in base esadecimale, il risultato
            all'interno del buffer 'client_id'.

        */
        sprintf(client_id, "%x", (int) ntohl((unsigned long) strtol(message, NULL, 16)));

        //Stampa di notifica del server (come richiesto dal progetto);
        printf("SERVER %d INCOMING FROM %s @ %ld\n", id, client_id, elapsed_from_init);

        /*
        In alcuni file del progetto si potrà trovare più volte il comando 'fflush(stdout)'.
        Ho inserito questo comando per far sì che lo stdout di 'supervisor' non sia bufferizzato. 
        Ho fatto questo perchè al momento dei test, lo stdout dei programmi viene indirizzato 
        all'interno di alcuni file di testo; se non mettessi questo comando, il file di testo 
        verrebbe scritto in maniera molto confusionaria, rendendo difficile la comprensione e 
        soprattutto l'interpretazione fatta da 'misura.sh' (vedere il file per chiarimenti).
        */
        fflush(stdout);

        //Dealloco 'message' per evitare memory leak
        free(message); 

    } 

    /*
        Il server arriva in questo punto una volta che il client ha chiuso la
        socket attraverso il quale comunicavano.

        La prima cosa che faccio e' salvare, convertendo quelli che erano microsecondi in millisecondi,
        il secret stimato nella variabile 'estimate' (bisogna convertire a 'int' poiche' 'best_sub' era 
        di tipo 'long').

        Una volta fatto cio' stampo il messaggio di notifica richiesto dal progetto, con una piccola 
        personalizzazione: se il server non ha ricevuto alcun messaggio dal client (il client potrebbe
        aver sempre scelto altri server cui era collegto) stampo una notifica differente, non avendo 
        dati utili per la notifica classica (per capire se ci sono stati messaggi, testo il valore di
        'client_id', l'ha mai ricevuto? Se si, ha salvato li dentro il valore).
    */

    estimate = (int) (best_sub / 1000);

    /*
        In alcuni file del progetto si potrà trovare più volte il comando 'fflush(stdout)'.
        Ho inserito questo comando per far sì che lo stdout di 'supervisor' non sia bufferizzato. 
        Ho fatto questo perchè al momento dei test, lo stdout dei programmi viene indirizzato 
        all'interno di alcuni file di testo; se non mettessi questo comando, il file di testo 
        verrebbe scritto in maniera molto confusionaria, rendendo difficile la comprensione e 
        soprattutto l'interpretazione fatta da 'misura.sh' (vedere il file per chiarimenti).
    */
    if(strcmp(client_id, "") == 0) {
        printf("SERVER %d CLOSING WITHOUT ANY MESSAGE FROM CLIENT\n", id);
        fflush(stdout);
    } else {
        printf("SERVER %d CLOSING %s ESTIMATE %d\n", id, client_id, estimate);
        fflush(stdout);
    }
    
    /*
        Il server procede ora alla comunicazione della stima fatta al supervisor.

        Viene allocato 'to_super', ci si scrive dentro il messaggio, della forma
        "<ID> <STIMA> <NUMERO_SERVER>"

        Successivamente lo realloco della size esatta per evitare sprechi.

        Infine invio il messaggio al supervisor; ATTENZIONE!!! Il messaggio
        viene inviato se e solo se il server ha ricevuto piu' di un messaggio!
            - se ha ricevuto un solo messaggio, la stima sara' 0, vedere commento alla riga 262;
            - se non ha ricevuto messaggi, la stima sara' 0, chiaramente;
    */

    to_super = (char*) _calloc(100, sizeof(char));
    sprintf(to_super, "%s %d %d", client_id, estimate, id);
    to_super = (char*) realloc(to_super, strlen(to_super) + 1);
    if(strcmp(client_id, "") != 0 && estimate != 0) {
        write(pfd, to_super, strlen(to_super) + 1);
    }

    //Libero il buffer appena alloca 'to_super'
    free(to_super);
    
    //Chiudo la socket anche lato server (se siamo arrivati qui, lato client e' gia' stata chiusa)
    close(socket_client);

    //Setto il thread attuale come terminato nell'array 'finished_threads'
    finished_threads[thread_index] = 1;

    //Libero la memoria allocata per il recupero degli argomenti
    free(args);

    //Termino il thread
    pthread_exit(NULL);
    
}


int main(int argc, char* argv[]) {

    /*
        Dichiarazione di alcune variabili.

        fd_skt --> intero in cui verra' salvato il file descriptor della socket "listener" del server;
        fd_c --> intero in cui verra' salvato il file descriptor della socket relativa ad una connessione;
                 se ne utilizza uno che verra' ogni volta sovrascritto perche' tanto il valore viene affidato 
                 alla 'struct to_thread' e passato al thread.
        sa --> variabile di tipo 'struct sockaddr_un'; serve per gestire il meccanismo delle socket e fare la corretta
               'bind' del server all'indirizzo prefissato, ovvero "OOB-server-[i]"; in poche parole, assegna un indirizzo
               alla socket fd_skt in modo tale che i client arrivino ad essa attraverso l'indirizzo stesso.
        socket_name --> buffer all'interno del quale scriveremo l'indirizzo (che sara' di tipo 'char*' dal momento
                        che utilizziamo come dominio 'AF_UNIX') della socket;
    */
    int fd_skt;
    int fd_c; 
    struct sockaddr_un sa;
    char socket_name[UNIX_PATH_MAX];
    fflush(stdout);

    /*
        In questa parte di codice si ha la gestione dei segnali e quindi l'installazione
        dei gestori. 

        Le operazioni sono le stesse fatte nel 'supervisor', ma in server ho reputato 
        necessario gestire due tipi di 'struct sigaction'. 
        
        s --> riguarda la gestione del segnale SIGTERM che il 'supervisor' invia ai vari server
              per permettere la loro corretta terminazione. Il gestore che viene installato e'
              'manage_function', funzione che e' stata implementata e commentata appena prima del main.

        reset --> ho deciso di inserire quest'altra 'sigaction'; questo perche' in 'supervisor' l'installazione
                  del gestore, e quindi la differenziazione per il segnale SIGINT, viene fatta PRIMA della fork().
                  Questo vuol dire che i server erediteranno quel cambiamento! Ho deciso quindi di resettare
                  il comportamento in caso di SIGINT per il server, ripristinando quello di default, 
                  tramite l'installazione del gestore (s.sa_handler = gestore) SIG_IGN;
    */
    struct sigaction s, reset;
    memset(&s, 0, sizeof(s));
    memset(&reset, 0, sizeof(reset));
    s.sa_handler = manage_function;
    reset.sa_handler = SIG_IGN;
    sigaction(SIGTERM, &s, NULL);
    sigaction(SIGINT, &reset, NULL);
    
    /*
        Il server riceve in input due argomenti: l'id del server (numero progressivo
        che lo identifica) e 'pfd', ovvero il descrittore della pipe sul quale scrivere
        per comunicare con 'supervisor'.

        Questi due argomenti non sono pero' passati dall'utente, bensi' direttamente da 'supervisor'.
        Per questo motivo non ho creato nessuna funzione ausiliaria di controllo, non possono esserci
        errori! 

        (per 'supervisor' e 'client' ho invece creato una funzione 'check-input-*' poiche
        l'input viene inserito da un utente, il quale puo sbagliare; consultare 'aux-functions.c' per
        ulteriori chiarimenti)
    */
    id = strtol(argv[1], NULL, 10) + 1;
    pfd = strtol(argv[2], NULL, 10);

    //Procedo all'inizializzazione dell'indirizzo, assegnandogli il nome 'OOB-server-[i]'
    sprintf(socket_name, "OOB-server-%d", id);
    strncpy(sa.sun_path, socket_name, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;


    //Messaggio di notifica (come richiesto dal progetto)
    printf("SERVER %d ACTIVE\n", id);

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
        Procedo all'apertura della "socket listener"; quest'operazione viene fatta 
        tramite la funzione '_socket', ovvero una ridefinizione della classica 'socket'. 

        Una volta fatto cio' faccio la '_bind' utilizzando l'indirizzo precedentemente 
        configurato e poi la '_listen'.

        Per quanto riguarda le funzioni '_socket', '_bind', '_listen' consultare
        il file 'err-managements.c' per ulteriori chiarimenti;
    */
    fd_skt = _socket();
    _bind(fd_skt, sa);
    _listen(fd_skt);

    int i = 0;
    while(1) {
        /*
            Il serve si mette in attesa di nuove connessioni da parte del client;

            Quando una connessione arriva, viene create (ed allocata) la struct
            'to_thread' per passare al thread gli argomenti necessari. 
            Per ulteriori chiarimenti leggere i commenti relativi alla struct alla riga 83.

            Una volta inizializzata la struct, viene effettivamente lanciato il thread,
            attraverso la funzione '_create_thread'. Per ulteriori chiarimenti riguardo
            la funzione, consultare il file 'err-managements.c'.

        */
        fd_c = _accept(fd_skt);
        struct to_thread* args = (struct to_thread*) _calloc(1, sizeof(struct to_thread));
        args->fd_c = fd_c;
        args->index = i;
        _create_thread(&(tid[i]), worker, args);
        i++;
        //Messaggio di notifica (come richiesto dal progetto)
        printf("SERVER %d CONNECT FROM CLIENT\n", id);
        /*
            In alcuni file del progetto si potrà trovare più volte il comando 'fflush(stdout)'.
            Ho inserito questo comando per far sì che lo stdout di 'supervisor' non sia bufferizzato. 
            Ho fatto questo perchè al momento dei test, lo stdout dei programmi viene indirizzato 
            all'interno di alcuni file di testo; se non mettessi questo comando, il file di testo 
            verrebbe scritto in maniera molto confusionaria, rendendo difficile la comprensione e 
            soprattutto l'interpretazione fatta da 'misura.sh' (vedere il file per chiarimenti).
        */
        fflush(stdout);
    }
    return 0;
}