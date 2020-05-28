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

#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<errno.h>

#include"err-managements.h"

/*
    Ho deciso di creare tre file esterni per raccogliere 
    alcune funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione.
    La spiegazione di ognuno si trova nei rispettivi file '.c' e '.h'

    Il seguente file e' 'err-managements.c' e implementa (e quindi #include) l'interfaccia 'err-managements.h'

    Ogni funzione di sistema (e non) prevede una notifica dell'errore spesso basata sul valore che 
    questa ritorna. Sarebbe necessario, o comunque preferibile, ogni qual volta una di queste viene chiamata,
    fare dei controlli sul valore di ritorno in modo da gestirne il comportamento in caso di errore.

    Non spieghero' singolarmente ogni funzione, per il semplice fatto che ogni ridefinizione e' stata
    fatta seguendo lo stesso schema.
    Supponiamo di voler ridefinire la funzione SYS, che prende in input a e b, e ritorna -1
    in caso di errore e 0 in caso di successo (oltre ad agire effettivamente).

    La ridefinizione e' stata fatta in questo modo: 
    
    _SYS(a, b) {    
        ...
        int flag = SYS(a, b);
        if(flag == -1) {
            perror("Messaggio di errore");
            exit(EXIT_FAILURE);
        }
        ...
    }

    Per fare le ridefinizioni, e' stato seguito il manuale per ogni funzione.

    NOTA BENE --> per alcune funzioni e' stata fatta la seguente semplificazione:
    Supponiamo che la stessa SYS precedentemente citata, nel nostro caso, verra' 
    usata sempre mettendo 'b' uguale a NULL o comunque ad una costante; la ridefinizione
    avra' interfaccia _SYS(a) e 'b' verra' settato direttamente all'interno
    dell'implementazione.

    NOTA BENE (2) --> un altro modo di arrivare a questo risultato prevede l'uso 
    di 'define' che controllino la presenza di errori nella chiamata. Ho pero' preferito
    utilizzare la tecnica delle ridefinizioni per preferenza personale.
*/


void* _calloc(size_t nmemb, size_t type) {
    void* pointer;
    pointer = calloc(nmemb, type);
    if(pointer == NULL && ((int) nmemb != 0) && ((int) type != 0)) {
        perror("Calloc failed!");
        exit(EXIT_FAILURE);
    }
    return pointer; //può essere NULL se nmemb o type == 0 (e non sarebbe errore)
}

void* _malloc(size_t size) {
    void* pointer;
    pointer = malloc(size);
    if(pointer == NULL && ((int) size) != 0) {
        perror("Malloc failed!");
        exit(EXIT_FAILURE);
    }
    return pointer;
}

int _fork() {
    int pid;
    pid = fork();
    if(pid < 0) {
        perror("Fork failed!");
        exit(EXIT_FAILURE);
    }
    return pid;
}

void _execvp(char* args[]) {
    execvp(args[0], args);
    perror("Execvp failed!");
    exit(EXIT_FAILURE);
}

void _pipe(int* pfd) {
    int flag;

    flag = pipe(pfd);
    if(flag == -1) {
        perror("Pipe creation failed!");
        exit(EXIT_FAILURE);
    }
}

int _socket() {
    int fd;

    if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void _bind(int fd_skt, struct sockaddr_un sa) {
    if(bind(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        perror("Bind failed!");
        exit(EXIT_FAILURE);
    }
}

void _listen(int fd_skt) {
    if(listen(fd_skt, SOMAXCONN) == -1) {
        perror("Listen failed!");
        exit(EXIT_FAILURE);
    }
}

int _accept(int fd_skt) {
    int fd_c;
    while((fd_c = accept(fd_skt, NULL, 0)) == -1 && errno == EINTR) {   //necessario per evitare errore di "accept failed" causato
                                                                        //da un'interruzione (errno = EINTR)
        continue;
    } 
    if(fd_c == -1) {
        perror("Accept failed!");
        exit(EXIT_FAILURE);
    }

    return fd_c;
}

void _connect(int fd_skt, struct sockaddr_un sa) {
    while(connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        if(errno == ENOENT) sleep(1);
        else {
            perror("Socket connection error!");
            exit(EXIT_FAILURE);
        }
    }
}

void _create_thread(pthread_t* tid, void *(*start_routine)(void*), void* arg) {
    int flag;
    
    flag = pthread_create(tid, NULL, start_routine, arg);
    if(flag != 0) {
        perror("Thread creation error!");
        exit(EXIT_FAILURE);
    }
}

void _join_thread(pthread_t tid, void** status_ptr) {
    int flag;
    while((flag=pthread_join(tid, status_ptr) != 0) && errno == EINTR) {
        continue;
    }
    if(flag != 0) {
        perror("Thread join error!");
        exit(EXIT_FAILURE);
    }
}   

int _read(int fd, void* buf, size_t count){
    int check;
    if((check = read(fd, buf, count)) <= 0) {
        if(check == -1) {
            printf("An error occurred!\n");
        } else {
            //printf("File descriptor is closed!\n");
        }
    }
    return check;
}

void _gettimeofday(struct timeval *tv) {
    if(gettimeofday(tv, NULL) == -1) {
        perror("Taken time error!");
        exit(EXIT_FAILURE);
    }
}