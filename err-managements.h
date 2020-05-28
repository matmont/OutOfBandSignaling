/*
    Matteo Montalbetti
    MAT. 559881
    Corso B
    "OUT-OF-BAND SIGNALING"
*/

#ifndef ERRMANAGEMENTS_H_INCLUDED
#define ERRMANAGEMENTS_H_INCLUDED

/*
    Le seguenti define sono state inserite per
    ovviare ad errori/warning durante la compilazione;
*/

#define _POSIX_C_SOURCE 200809L

#include<sys/un.h>
#include<sys/time.h>
#include<pthread.h>
#include<sys/types.h>

/*
    Ho deciso di creare tre file esterni per raccogliere 
    alcune funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione.
    La spiegazione di ognuno si trova nei rispettivi file '.c' e '.h'

    Il seguente file e' 'err-managements.h' e contiene l'interfaccia per 'err-managements.c'

    Ogni funzione di sistema (e non) prevede una notifica dell'errore spesso basata sul valore che 
    questa ritorna. Sarebbe necessario, o comunque preferibile, ogni qual volta una di queste viene chiamata,
    fare dei controlli sul valore di ritorno in modo da gestirne il comportamento in caso di errore.

    Le varie funzioni sono spiegate nel file 'err-managements.c'
*/

void* _calloc(size_t nmemb, size_t type);

void* _malloc(size_t size);

int _fork();

void _execvp(char* args[]);

void _pipe(int* pfd);

int _socket();

void _bind(int fd_skt, struct sockaddr_un sa);

void _listen(int fd_skt);

int _accept(int fd_skt);

void _connect(int fd_skt, struct sockaddr_un sa);

void _create_thread(pthread_t* tid, void* (*start_fcn) (void *), void* arg);

void _join_thread(pthread_t tid, void** status_ptr);

int _read(int fd, void* buf, size_t count);

void _gettimeofday(struct timeval *tv);

#endif