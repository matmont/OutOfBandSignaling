/*
    Matteo Montalbetti
    MAT. 559881
    Corso B
    "OUT-OF-BAND SIGNALING"
*/

#ifndef PIPES_H_INCLUDED
#define PIPES_H_INCLUDED

/*
    Ho deciso di creare tre file esterni per raccogliere 
    alcune funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione.
    La spiegazione di ognuno si trova nei rispettivi file '.c' e '.h'

    Il seguente file e' 'pipes.h' contenente l'interfaccia per 'pipes.c'

    "Mi serviva una struttura dati che potesse tenere traccia
    di tutti i descrittori dei file relativi alle pipes create. 
    La creazione, gestione ed eliminazione di questo array è responsabilità 
    di questo file." 

    Le varie funzioni sono spiegate nel file 'pipes.c'
*/

int** create_pipes(int k);

void free_pipes(int** pipes, int k);

void print_values(int** pipes, int size);

int server_fd(int** pipes, int fd, int size);

#endif