/*
    Matteo Montalbetti
    MAT. 559881
    Corso B
    "OUT-OF-BAND SIGNALING"
*/

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>


#include"pipes.h"
#include"err-managements.h"

/*
    Ho deciso di creare tre file esterni per raccogliere 
    alcune funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione.
    La spiegazione di ognuno si trova nei rispettivi file '.c' e '.h'

    Il seguente file e' 'pipes.c', che implementa (e quindi #include)
    l'interfaccia 'pipes.h'.

    "Mi serviva una struttura dati che potesse tenere traccia
    di tutti i descrittori dei file relativi alle pipes create. 
    La creazione, gestione ed eliminazione di questo array è responsabilità 
    di questo file."


    L'idea e' la seguente:

            [ [pfd0[0], pfd0[1] ], [pfd1[0], pfd1[1] ], ..., [pfdN[0], pfdN[1] ]  ]

*/

/*
    @param size --> (int) la lunghezza di cui creare l'array;
    @return (int**) ritorna il puntatore alla struttura appena creata;

    La funzione 'create_pipes' serve ad inizializzare l'array 'pipes'. Viene allocata
    la memoria in base alla 'size' (che sara' poi il parametro k del supervisor), e per
    ogni elemento viene allocata ulteriore memoria per inizializzare un array di 2 interi 
    (descrittore della pipe per la scrittura e per la lettura).

    Entrambe le allocazioni vengono fatte tramite la funzione ridefinita in 'err-managements.h'
    '_calloc'. Per ulteriori chiarimenti riguardo a questa consultare 'err-managements.h'
*/
int** create_pipes(int size) {
    int** pipes;
    pipes = _calloc(size, sizeof(int*));
    for (int i = 0; i < size; i++)
    {
        *(pipes+i) = _calloc(2, sizeof(int));
    }
    
    return pipes;
}

/*
    @param pipes --> (int**) l'array da deallocare;
    @param size --> (int) la lunghezza dell'array;

    La funzione 'free_pipes' serve ad eliminare, deallocando correttamente la memoria,
    la struttura dati 'pipes'.

    Per ogni elemento di 'pipes', quindi per i k array di due descrittori, si 
    procede alla chiusura di questi, e alla 'free' dell'array;

    Infine si fa una 'free' dell'intera struttura 'pipes'
*/
void free_pipes(int** pipes, int size){
    for (int i = 0; i < size; i++)
    {
        close((*(pipes+i))[0]);
        close((*(pipes+i))[1]);
        free(*(pipes+i));
    }
    free(pipes);
}

/*
    @param pipes --> (int**) l'array da stampare;
    @param size --> (int) la lunghezza dell'array;

    La funzione 'print_values' serve a stampare il contenuto di 'pipes'.

    Viene semplicemente scandito l'array stampando per ogni elemento un riga di info.

    LA FUNZIONE NON E' STATA UTILIZZATA E NON COPRE UN UTILE SCOPO NEL PROGETTO

    (E' stata creata per utilita' in fase di debugging)
*/
void print_values(int** pipes, int size) {
    printf("---Value in the pipes queue---\n");
    for (int i = 0; i < size; i++)
    {
        printf("Pipe Server %d --> ", i + 1);
        printf("[0]: %d | [1]: %d \n", (*(pipes+i))[0], (*(pipes+i))[1]);
    }
}

/*
    @param pipes --> (int**) l'array 'pipes';
    @param fd --> (int) file descriptor da cercare;
    @param size --> (int) la lunghezza dell'array;
    @return (int) l'indice del server collegato al descrittore
                  'fd'. Nel caso in cui nessun server sia collegato
                  a quel descrittore, la funzione ritorna -1;
    
    La funzione 'server_fd' ritorna l'indice del server corrispondente
    "collegato" al descrittore (o meglio alla pipe avente da uno dei due 
    lati quel descrittore) passato come input alla funzione.
*/
int server_fd(int** pipes, int fd, int size) {
    int server_index = -1;
    for (int i = 0; i < size; i++)
    {
        if((*(pipes+i))[0] == fd) {
            server_index = i + 1;
            break;
        }
    }
    return server_index;
}