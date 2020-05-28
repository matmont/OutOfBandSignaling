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

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<time.h>
#include<string.h>
#include<unistd.h>
#include<sys/select.h>
#include<signal.h>
#include<bits/sigaction.h>
#include<sys/wait.h>
#include<errno.h>

/*
    Ho deciso di creare tre file esterni per raccogliere alcune 
    funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione. La spiegazione di ognuno e' 
    commentata nei rispettivi file '.c'e '.h';
*/
#include"aux-functions.h"
#include"err-managements.h" 
#include"pipes.h"


/*  RICHIESTA DEL PROGETTO:
    All'avvio, il supervisor dovra stampare sul suo stdout un messaggio “SUPERVISOR STARTING k”, in cui k
    e il numero di server da lanciare (che il supervisor riceve come argomento sulla riga di comando). A questo
    punto, dovra lanciare (come processi distinti) i k server, e mantenere con ciascun server una connessione
    tramite pipe anonime con cui potra ricevere le stime dei secret dei client da parte dei server. Per ogni nuova
    stima ricevuta, il supervisor dovra stampare su stdout un messaggio “SUPERVISOR ESTIMATE s FOR id
    FROM i”, in cui s e il valore della stima del secret, id e l'ID del client a cui si riferisca la stima, e i e l'indice
    del server che ha fornito la stima.
    Il supervisor ricevera ovviamente stime da piu server per lo stesso client. Deve quindi utilizzare le diverse
    stime arrivate fino a un dato momento per determinare il valore piu probabile del secret “vero”; chiamiamo
    questo valore Sid. Quando il supervisor riceve un segnale SIGINT, deve stampare sul suo stderr una tabella
    che mostri le sue stime correnti, in cui ciascuna riga ha il formato “SUPERVISOR ESTIMATE Si dFOR id
    BASED ON n” – in cui n rappresenta il numero di server che hanno fornito stime per i d fino a quel
    momento. Se invece riceve un “doppio Ctrl-C”, ovvero un secondo segnale SIGINT a distanza di meno di un
    secondo dal precedente, il supervisor deve terminare, stampando prima la stessa tabella su stdout seguita dal
    messaggio “SUPERVISOR EXITING”. 
*/

/*
    Il supervisor ha bisogno di tenere traccia, durante la sua esecuzione, di alcuni dati:
    le stime ricevute, i client cui queste si riferiscono e un contatore che indichi il numero di server
    sul quale e' basata la stima; per fare cio' ho deciso di utilizzare, ed implementare, una sorta di
    linked-list.
    
    Qui sotto troviamo la definizione delle due struct necessarie: la prima definisce la natura dell'
    informazione che ogni nodo della lista conterra', la seconda definisce la natura di ogni nodo. 
    La struct 'data' conterra' chiaramente le tre informazioni indicate nel paragrafo appena sopra, 
    mentre la struct 'node' avra' un puntatore ad una struct 'data' e un puntatore ad un 'node' 
    successivo, cosi' da creare la lista;
*/
struct data {
    unsigned long id;
    int estimate;
    int counter;
    
};

struct node {
    struct data* info;
    struct node* next;
};

/*
    Inizializzo la lista nel quale il supervisor immagazzinera' i dati; la inizializzo
    a NULL e verra' successivamente popolata tramite le varie funzioni definite dopo;
*/

struct node* list = NULL;

/*
    Dichiarazione di alcune variabili; ho deciso di dichiararle qui (globalmente) in modo
    tale che le varie funzioni esterne al 'main' possano accedervi e lavorarci senza problemi;

    Le prime tre sono utilizzate per immagazzinare i dati provenienti dai server; una volta che 
    viene letto il messaggio nel buffer di ricezione, il supervisor invoca la funzione 'tokenizer'
    che si occupa dell'interpretazione (recuperando le tre informazioni tramite 'strtok') del 
    messaggio ricevuto, immagazzinando i dati nelle rispettive variabili;

    ... [continua nel successivo]
*/
unsigned long received_id;
int received_estimate;
int received_server;

/*
    ...
    Per quanto riguarda invece le seguenti due variabili: abbiamo 'on' che regola il funzionamento del
    'supervisor' in quanto guardia del while (quando 1 il while scorre, quando 0 si ferma); mentre 'last_signal'
    serve per tenere traccia del momento (in termini di tempo) in cui il supervisor ha ricevuto l'ultimo SIGINT (CTRL+C);
*/

volatile sig_atomic_t on = 1; 
time_t last_signal = 0;

/*
    Le seguenti 6 funzioni sono le funzioni chiave che ho deciso di creare per implementare
    la linked-list; abbiamo infine 'tokenizer' che, come detto sopra, si occupa dell'interpretazione
    dei messaggi ricevuti dai server.

    In questa fase del codice ho semplicemente deciso di dichiararle, senza pero' implementarle; la spiegazione
    di ognuna e' stata fatta in fondo al file (sempre tramite commenti), dove effettivamente sono state poi implementate.

*/
void add_element(unsigned long id, int estimate);   //aggiunta in coda
void new_estimate(unsigned long id, int estimate);
int member_id(unsigned long id);       //1 se c'è, 0 altrimenti
void print_table();
void print_table_err();
void delete_list();

void tokenizer(char* message);

/*
    'manage_function' e' la funzione chiamata in causa nel momento in cui il programma
    'supervisor' riceve un segnale di tipo SIGINT. Quello che fa e' molto semplice: e' presente
    un controllo, in termini di tempo, rispetto al segnale ricevuto precedentemente; se il segnale appena
    intercettato e' arrivato dopo 1 secondo o meno rispetto all'ultimo, il supervisor deve terminare la sua esecuzione,
    non prima pero' di aver fatto alcune operazioni. Per fare questo ho semplicemente settato la "variabile di accesione" 'on'
    a 0, cosi che il ciclo while si concluda. Successivamente al ciclo, all'interno del 'main', verranno eseguite le 
    ultime operazioni, che vedremo dopo.

    Se, in caso contrario, il nuovo segnale e' arrivato dopo piu' di 1 secondo rispetto al precedente, cio' che 'supervisor'
    deve fare e' stampare la propria tabella (la linked-list di cui abbiamo gia parlato) sul suo 'stderr'. 
    Questa operazione e' eseguita dalla funzione 'print_table_err()' che vedremo piu' avanti;

    Infine, la funzione aggiorna il valore di 'last_signal' inserendo il tempo attuale.
*/
static void manage_function(int signum) {

    if(time(NULL) - last_signal <= 1) {
        on = 0;
    } else {
        print_table_err();
    }
    
    last_signal = time(NULL);

}

/*
    Inizia qui il main del 'supervisor'. E' pero' consigliato saltare prima alla fine di questo, 
    dove vengono implementate e spiegate le varie funzioni ausiliare utilizzate poi all'interno del main;
*/
int main(int argc, char* argv[]) {

    /*
        La prima sezione del codice è dedicata alla dichiarazione di alcune variabili. Vediamole una per una:

        k --> rappresenta il numero di server che il supervisor dovrà lanciare. Il contenuto della
              sarà "recuperato" dall'input inserito al momento del lancio del programma;
        pid --> è una variabile di appoggio che si utilizza quando si fa uso della 'fork', così
                da poter contorollarne il valore per differenziare tra padre e figlio. Inoltre,
                i vari 'pid' saranno salvati nell'array 'childs';
        childs --> è un array di interi; lo utilizzo per memorizzare il 'pid' di tutti i processi
                   figli, così da poterli terminare in maniera corretta una volta finita l'esecuzione;
        pipes --> è un array di array di interi. Mi serviva una struttura dati che potesse tenere traccia
                  di tutti i descrittori dei file relativi alle pipes create. La creazione, gestione ed 
                  eliminazione di questo array è responsabilità del file 'pipes.c' (e 'pipes.h'). Per
                  ulteriori chiarimenti consultare i commenti nel file stesso;
        rd_set, tmp_set --> sono due variabili di tipo 'fd_set'; rappresentano un set nel meccanismo della
                            'select'. Ma perchè ne ho definiti due? 'rd_set' è chiaramente il set di lettura,
                            quello su cui effettivamente la 'select' lavorerà; il problema però sta nel fatto
                            che la 'select' provoca alcune modifiche al set sul quale lavora, ed è quindi 
                            necessario avere un set di "supporto", che ogni volta userò per ripristinare
                            'rd_set' prima di essere passato alla 'select';
        fd_max, retval --> anche queste due variabili sono utilizzate nel meccanismo della 'select'. La prima
                           serve per tenere traccia del numero di file descriptor aperti, valore
                           che dev'essere passate alla 'select' per il suo corretto funzionamento (si calcola
                           da fd_max come segue --> fd_max + 1). La seconda rappresenta invece il valore tornato
                           ogni volta dalla 'select', il quale verrà testato per controllare la presenza di
                           eventuali errori che la 'select' potrebbe aver incontrato;
        s --> 's' è una variabile di tipo 'struct sigaction'. Una variabile di questo tipo serve a personalizzare
              la gestione di un determinato segnale tramite l'installazione di un "gestore", ovvero di una funzione
              che sarà eseguita nel momento in cui il segnale scelto verrà ricevuto dal processo. Per prima cosa si
              fa una memset per inizializzare a 0 tutti i valori della struct. Successivamente si modifica l'attributo
              'sa.handler' assegnandogli la funzione "gestore" da invocare (nel nostro caso la funzione 'manage_function' 
              di cui abbiamo parlato prima). Infine si completa l'installazione del gestore tramite il comando
              'sigaction', nel quale si indica il segnale soggetto dell'operazione, e il nuovo gestore per esso;
    */

    //General purpose variables
    int k;  
    int pid;
    int* childs;
    //Pipes variables
    int** pipes;
    //Select variables
    fd_set rd_set, tmp_set;
    int fd_max;
    int retval;
    //Signals variables
    struct sigaction s;
    memset(&s, 0, sizeof(s));
    s.sa_handler = manage_function;
    sigaction(SIGINT, &s, NULL);
    
    /*
        Gestione dell'input con il quale il programma è stato lanciato. Sostanzialmente quello che
        succede è la conversione dell'input (che avrà formato 'char*') in intero e assegnazione
        di questo valore alla variabile 'k' di cui abbiamo già parlato.

        E' presente anche un'altra funzione, ovvero 'check_supervisor_inputs'. Questa è una
        funzione ausiliaria che ho deciso di creare per mantenere il codice del supervisor più pulito. 
        Ciò che fa è controllare e notificare i vari errori che potrebbero esserci nell'input inserito; 
        nel caso dovessero esserci problemi, il programma viene immediatamente terminato tramite una 'exit(EXIT_FAILURE)' 

        Per ulteriori chiarimenti sull'implementazione della funzione, consultare i commenti
        contenuti nel file 'aux-functions.c';
    */
    check_supervisor_inputs(argc, argv);
    k = strtol(argv[1], NULL, 10);
    
    /*
        Viene inizializzata la variabile 'pipes' di cui abbiamo già parlato. Viene quindi
        allocata la memoria ed effettivamente creato l'array di cui parlavo precedentemente, così
        da poterci memorizzare all'interno i vari descrittori di file delle pipes.

        Per ulteriori chiarimenti consultare il file 'pipes.c';
    */
    pipes = create_pipes(k);
    

    //Messaggio di "start" del supervisor (come richiesto dal progetto)
    printf("SUPERVISOR STARTING %d\n", k);

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
        Inizializzazione dell'array 'childs'. Qui andremo a memorizzare i vari
        pid dei processi figli, ovvero dei vari server creati.
    */
    childs = (int*) _calloc(k, sizeof(int));

    /*
        Il seguente ciclo for è utilizzato per creare k processi figli,
        che saranno poi i k server.
    */
    for (int i = 0; i < k; i++)
    {
        /*
            '_pipe' è la ridefinizione del metodo 'pipe'. Consultare il file
            'err-managements.c' per ulteriori chiarimenti. In generale però, ho ridefinito
            la funzione per gestire gli errori esternamente al supervisor e tenere il 
            codice il più pulito possibile. 

            Come argomento il metodo 'pipe' prende un array di 2 interi (dove andrò a memorizzare
            i due descrittore, pipe lettura e scrittura): 'pipes' è proprio un array di array di 2 interi.

            Quindi questo metodo fa sì che all'i-esimo ciclo, quando viene creato l'i-esimo server,
            pipes[i-1] = [pipe con l'i-esimo server in lettura, pipe con l'i-esimo server in scrittura]
        */
        _pipe(*(pipes+i));

        /*
            In questo momento si crea il nuovo processo tramite il metodo '_fork', una
            redifinizione della classica 'fork'. Per ulteriori chiarimenti consultare il foglio
            'err-managements.c'.

            Si procede all'inserimento del pid nell'apposito array.
        */
        pid = _fork();
        *(childs+i) = pid;

        if(pid == 0) {
            /*
                Ciò che è scritto qui viene eseguito solo dal figlio (pid == 0). Sostanzialmente
                avviene la vera e propria differenziazione tramite il metodo '_execvp', una redifinizione
                del classico 'execvp'. Per ulteriori chiarimenti consultare il foglio
                'err-managements.c'.

                Prima di eseguire la differenziazione è necessario preparare alcuni dati che saranno
                passati come input al lancio del programma 'server'; questi dati sono rispettivamente
                il numero del server (sequenziale), e il descrittore della pipe PER LA SCRITTURA (per le
                operazioni che deve fare il server è necessaria solo questa) sul quale dovrà comunicare
                le stime al supervisor.
            */
            char number[100];
            char pfd[100];

            sprintf(number, "%d", i);
            sprintf(pfd, "%d", (*(pipes+i))[1]);

            char* args[] = {"./server", number, pfd, NULL};
            _execvp(args);
        }
    }
    
    /*
        Inizia qui l'implementazione del meccanismo della 'select'. Per prima cosa
        si inizializza fd_max a 0 e soprattutto si inizializza il set 'tmp_set', che 
        andremo poi a popolare.
    */
    fd_max = 0;
    FD_ZERO(&tmp_set);
    
    /*
        Si procede ora alla popolazione del set temporaneo (precedentemente abbiamo parlato del 
        motivo per il quale è necessario averne uno). Quello che si fa è inserire nel set tutti
        i descrittori delle pipe PER LA LETTURA (il supervisor utilizza le pipe solo in lettura).
        *** N.B --> 'pipes' è lungo k, poichè k sono i server ***

        Oltre all'inserimento viene anche fatto un controllo di aggiornamento di 'fd_max';
    */
    for (int i = 0; i < k; i++)
    {  
        FD_SET((*(pipes+i))[0], &tmp_set);
        if((*(pipes+i))[0] > fd_max) fd_max = (*(pipes+i))[0];
    }

    /*
        Il ciclo continuo del supervisor. Fino a quando la variabile d'accensione
        'on' rimane a 1, il supervisor continuerà a rimanere in ascolto sulle pipe in
        attesa di nuove stime. Consultare i commenti relativi alla funzione gestore 
        per chiarimenti sulla modifica della variabile 'on'.
    */
    while(on){
        /*
            Prima abbiamo parlato del motivo per cui è necessario avere un set 
            temporaneo. Questo è il momento in cui "ripristiniamo" 'rd_set' 
            dalle modifiche che la 'select' comporta su di esso.
        */
        rd_set = tmp_set; 
        //Chiamata della 'select'. Non vogliamo nessun timer!
        retval = select(fd_max + 1, &rd_set, NULL, NULL, NULL);
        
        if(retval == -1) {
            //error for select
        } else {
            /*
                La 'select' ha notificato la presenza di un canale pronto per operazioni di I/O.
                Si procede quindi al check di tutti i descrittori da 0 fino a 'fd_max' testando
                tramite la 'FD_ISSET' l'appartenenza al set 'rd_set' e l'effettivo esser pronto del
                descrittore (o dei descrittori!);

                Una volta individuato un descrittore consono, si procede alla lettura di esso, e quindi
                nel nostro caso alla lettura della pipe. 
            */
            for (int fd = 0; fd <= fd_max; fd++)
            {
                if(FD_ISSET(fd, &rd_set)) {
                    /*
                        Alloco un buffer per la ricezione del messaggio.

                        Come si può notare, utilizzo una funzione preceduta da '_'. 
                        E' infatti la ridefinizione di 'calloc', che ho creato per il solito 
                        motivo, tenere più pulito il codice del supervisor, gestendo però gli
                        errori. Per ulteriori chiarimenti consultare il foglio 'err-managements.c'
                    */
                    char* message = (char*) _calloc(100, sizeof(char));
                    
                    /*  
                        Voglio salvare il valore tornato dalla read in modo da poter
                        eseguire alcuni controlli sull'esito dell'operazione.

                        Non è necessario controllare il caso in cui 'nread' sia minore 
                        di 0 in uscita da '_read' poichè, essendo un caso di errore, è
                        già coperto dalla ridefinizione della funzione. Per ulteriori 
                        chiarimenti consultare il foglio 'err-managements.c'
                    */
                    int nread = _read(fd, message, 100);
                    if(nread == 0) {
                        //Se dovessimo entrare in questo ramo, vorrebbe dire che la pipe è stata chiusa lato server!
                        //Impossibile, non ci entreremo mai.
                        printf("Pipe has been closed by server!");
                    } else if(nread < 0) {
                        //Errore! E' pero' gestito in 'err-managements.c'
                    } else {
                        /*
                            La lettura è andata a buon fine; invochiamo allora la funzione 'tokenizer' (per ulteriori
                            chiarimenti consultare le funzioni spiegate dopo il 'main') passandole come parametro
                            il buffer dove è contenuto il messaggio, così che possa assegnare i valori alle variabili
                            corrispondenti di cui abbiamo parlato all'inizio.
                        */
                        tokenizer(message);

                        //Stampa della riceazione di una nuova stima (come richiesto dal progetto)
                        printf("SUPERVISOR ESTIMATE %d FOR %x FROM %d\n", received_estimate, (int) received_id, received_server);
                        
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
                            Per prima cosa controllo tramite la funzione 'member_id' la presenza o meno, nella
                            tabella dei dati del supervisor, dell'id del client cui riferisce la stima ricevuta 
                            (e quindi la presenza di stime per quel client). 

                            Se il client dovesse già essere nella tabella viene invocata la funzione
                            'new_estimate' per valutare la necessità di aggiornare o meno la stima.
                            In caso contrario si invoca la funzione 'add_element' per aggiungere
                            il nuovo client, con la relativa stima appena ricevuta, nella tabella (che ho
                            gestito come una linked-list).

                            Per ulteriori chiarimenti riguardo le funzioni appena citate consultare i commenti
                            successivi al 'main', dove queste vengono implementate.
                        */
                       if(member_id(received_id)) { 
                           new_estimate(received_id, received_estimate);
                       } else {
                           add_element(received_id, received_estimate);
                       }

                    }
                    //Libero la memoria che avevo allocato per il buffer di ricezione.
                    free(message);
                }
            }
            
        }

    }   
    
    /*
        Siamo fuori dal ciclo while. Questo vuol dire che sono stati lanciati
        due segnali SIGINT a distanza di 1 secondo o meno l'uno dall'altro.

        Ci avviamo quindi alla terminazione del processo 'supervisor'. Prima però 
        di terminare, deve:
            - (come richiesto da progetto) stampare SULLO STDOUT la tabella
              con i dati raccolti fino ad ora, e il messaggio di chiusura;
            - liberare correttamente TUTTA la memoria allocata, così da ridurre
              i possibili leak;
    */
    print_table();    

    printf("SUPERVISOR EXITING\n");
    fflush(stdout);

    //Eliminazione della linked-list. La funzione è implementata e spiegata dopo il 'main';
    delete_list();

    //Chiudo tutti i descrittori delle pipe
    for (int i = 0; i < k; i++)
    {
        close((*(pipes+i))[0]);
        close((*(pipes+i))[1]);
    }
    //Elimino (liberando la memoria da essa allocata) la struttura 'pipes'. 
    //Per ulteriori chiarimenti consultare il file "pipes.c"
    free_pipes(pipes, k);
    
    /*
        Mando un segnale (che verrà poi gestito dai server) di terminazione
        a tutti i processi figli che avevo creato (per questo abbiamo usato
        l'array 'childs') così che possano terminare in modo corretto, senza
        provocare la presenza di "zombie processes".
    */
    for (int i = 0; i < k; i++)
    {
        kill(*(childs+i), SIGTERM);
    }
    //Libero la memoria allocata per l'array 'childs'
    free(childs);

    fflush(NULL);

    return 0;
}

/*
    @param id --> (unsigned long) id del client soggetto della stima;
    @param estimate --> (int) stima;

    La funzione 'add_element' serve ad inserire un nuovo elemento nella linked-list. 
    E' un'aggiunta in coda; ho deciso di implementare solo questa poiche' (essendo indifferente)
    la trovo piu' semplice.

    Quello che fa questa funzione e' allocare, e quindi creare, un nuovo nodo, inserendo
    l'informazione relativa ad esso. I dati da inserire sono presi dalla funzione in input; chiaramente
    sono richiesti solo la stima e l'id del client cui questa si riferisce, poiche' il contatore viene
    inizializzato ad 1 quando un elemento e' AGGIUNTO. Una stima ricevuta puo' essere aggiunta 
    ma anche aggiornata, in base alla presenza o meno dell'id nella tabella. Si AGGIUNGE quando l'id 
    e' "nuovo" e quindi non ci sono ancora altre stime per esso (per questo si inizializza ad 1).

    Una volta definito e "popolato" il nuovo nodo, si controlla se la lista e' ancora vuota
    oppure ha degli elementi. Questo controllo serve perche' nel secondo caso c'e la necessita'
    di scorrerla tutta per arrivare in fondo, ed aggiungere cosi il nuovo nodo;
*/
void add_element(unsigned long id, int estimate) {
    /*
        Viene utilizzata la funzione '_malloc'. Questa funzione fa chiaramente cio
        che fa la 'malloc'; semplicemente l'ho ridefinita in 'err-managements.h' per
        coprire eventuali errori e lasciare cosi il codice piu' pulito;
    */
    struct node* new_node = (struct node*) _malloc(sizeof(struct node));
    struct node* tmp;

    new_node->info = (struct data*) _malloc(sizeof(struct data));
    new_node->info->id = id;
    new_node->info->estimate = estimate;
    new_node->info->counter = 1;
    
    new_node->next = NULL;

    if(list == NULL) {
        list = new_node;
    } else {
        tmp = list;
        while(tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new_node;
    }
}

/*
    @param id --> (unsigned long) id del client soggetto della stima;
    @param rec_estimate --> (int) stima ricevuta;

    La funzione 'new_estimate' definisce il comportamento del supervisor davanti
    alla ricezione di una stima per un client gia presente nella tabella dei dati;
    cio che questa funzione fa, in sostanza, e' decidere se la stima appena ricevuta andra' 
    ad aggiornare la stima gia' presente in memoria (qual e' la migliore tra le due).

    Per prima cosa crea un puntatore temporaneo, cosi' da poter
    scorrere la lista fino a posizionarsi sul nodo relativo all'id del client in questione.
    *** IMPORTANTE !!! Abbiamo la certezza riguardo la presenza di questo nodo, poiche' per
    "costruzione" del 'main', questa funzione viene eseguita se la 'member' (che vedremo dopo) 
    ne indica la presenza! (Altrimenti si esegue 'add_element') ***

    (*[1]) - Vediamo ora come il supervisor valuta le due stime a confronto. 
    Per come e' impostato il progetto, arrivati a questo punto, la stima migliore
    sara' sicuramente la stima minore tra le due (tutte le stime sono o
    approssimazioni del secret, o di un suo multiplo).
    Testando pero' un determinato numero di volte il mio progetto, ho potuto
    osservare che tutte le volte in cui una stima non e' stata fatta in modo corretto,
    l'errore e' di circa il doppio. Ovvero, se ad esempio il secret fosse 1600, in caso di
    errore verra' stimato 3200. 
    Ho deciso allora di inserire un ulteriore controllo: se la stima piu recente e' minore di 
    quella gia presente la tabella si aggiornera' sicuramente; se pero la nuova stima (pur essendo
    minore della precedente) e' maggiore di 3000, allora vado ad inserire la meta' di essa (il 
    secret non puo' essere maggiore di 3000, quindi la stima e' sicuramente sbagliata).
    
    Rimane pero' un problema. Che succede se il secret dovesse essere 400 e arriva la stima 800?
    In questo caso non posso inserire la meta', perche 800 potrebbe benissimo essere un altro secret!
    Questo rimane l'unico caso (basandomi sui test fatti) in cui si verifica un errore nella stima;
*/
void new_estimate(unsigned long id, int rec_estimate) {
    //Il puntatore temporaneo, per scorrere la lista;
    struct node* tmp = list;
    
    while(tmp->info->id != id) {
        tmp = tmp->next;
    }

    //Leggere il commento appena sopra per eventuali chiarimenti; (*[1])
    if(tmp->info->estimate > rec_estimate) {
        if(rec_estimate > 3000) {
            tmp->info->estimate = (rec_estimate / 2);
        } else {
            tmp->info->estimate = rec_estimate;
        }
    }
    //Incremento del contatore;
    tmp->info->counter = tmp->info->counter + 1;

}

/*
    @param id --> (unsigned long) id del client del quale si vuole controllare la presenza;
    @return (int) Ritorna un intero che funge da 'flag'
                    1 --> e' presenta una stima per l'id;
                    0 --> non e' presente una stima per l'id;

    La funzione 'member_id' serve a controllare la presenza o meno nella tabella del supervisor
    di stime per un certo client; sulla base del risultato di questa funzione, si decide 
    se lanciare la funzione 'add_element' o 'new_estimate'.

    Il funzionamento non varia dalla classica 'member': si scorre la lista fino a quando non si
    trova una corrispondenza;
*/
int member_id(unsigned long id) {
    struct node* tmp = list;

    while(tmp != NULL) {
        if(tmp->info->id == id) {
            return 1;
        }
        tmp = tmp->next;
    }
    return 0;
}

/*
    Le funzioni 'print_table' e 'print_table_err' fanno praticamente la stessa cosa; per questo
    ho deciso di scrivere un unico commento per entrambe.

    Cio che fanno e' stampare la tabella attuale in possesso dal supervisor: scorrono la lista e per 
    ogni nodo stampano le informazioni contenute in esso (seguendo il pattern fornito dal testo del progetto).

    L'unica differenza tra le due e' che la seconda stampa sullo stderr e NON sullo stdout.
    Quest'ultima verra' invocata quando il programma riceve un singolo SIGINT (come richiesto dal testo del progetto).
*/
void print_table() {
    struct node* tmp = list;
    while(tmp != NULL) {
        printf("SUPERVISOR ESTIMATE %d FOR %x BASED ON %d\n", tmp->info->estimate, (int) tmp->info->id, tmp->info->counter);
        tmp = tmp->next;
    }
}
void print_table_err() {
    struct node* tmp = list;
    while(tmp != NULL) {
        fprintf(stderr, "SUPERVISOR ESTIMATE %d FOR %x BASED ON %d\n", tmp->info->estimate, (int) tmp->info->id, tmp->info->counter);
        tmp = tmp->next;
    }
    
}

/*
    La funzione 'delete_list' e' l'ultima funzione del gruppo relativo alla linked-list.

    Serve ad "eliminare" la lista, liberando, in maniera corretta, la memoria allocata
    per e da essa. 

    Importante ricordare che non basta fare la 'free' dei vari nodi, ma anche del puntatore
    ai dati contenuto in ogni nodo.
*/
void delete_list() {
    struct node* tmp;
    while(list != NULL) {
        tmp = list;
        list = list->next;
        free(tmp->info);
        free(tmp);
    }
}

/*
    @param message --> (char*) messaggio ricevuto dal server;

    La funzione 'tokenizer' si occupa di interpretare il messaggio ricevuto dal server.

    Quello che fa e' spezzare il messaggio (della forma --> "<ID> <ESTIMATE> <SERVER_ID>"),
    aggiornando le variabili viste all'inizio del codice con le informazioni ricavate da esso.

    Non e' necessario preoccuparsi di un'eventuale gestione della concorrenza perche' la 
    lettura dalle pipe dei vari server e' gestita in modo sequenziale (e quindi NON parallelo)
    tramite la 'select'.

    Una volta che le tre variabili sono aggiornate, verra' lanciata (nel main) la funzione di aggiunta o 
    aggiornamento della tabella sulla base dei dati acquisiti; 
    Verranno poi ogni volta sovrascritte per ogni successivo messaggio.

    Per implementare la funzione ho utilizzato la classica funzione 'strtok' per spezzare il messaggio;
    ho invece utilizzato 'strtol' per convertire il testo in 'int' e 'unsigned long' 
    (notare l'utilizzo della base 16 per la prima 'strtol': la parte di messaggio relativa 
    consiste nella rappresentazione esadecimale dell'id!).
*/
void tokenizer(char* message) {
    //Si imposta come carattere separatore lo spazio
    char* token = strtok(message, " ");
    //Si utilizza la base 16 come terzo parametro di 'strtol'
    received_id = (unsigned long) strtol(token, NULL, 16);
    token = strtok(NULL, " ");
    received_estimate = (int) strtol(token, NULL, 10);
    token = strtok(NULL, " ");
    received_server = (int) strtol(token, NULL, 10);
}