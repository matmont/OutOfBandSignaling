/*
    Matteo Montalbetti
    MAT. 559881
    Corso B
    "OUT-OF-BAND SIGNALING"
*/

#ifndef AUXFUNCTIONS_H_INCLUDED
#define AUXFUNCTIONS_H_INCLUDED

/*
    Ho deciso di creare tre file esterni per raccogliere 
    alcune funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione.
    La spiegazione di ognuno si trova nei rispettivi file '.c' e '.h'

    Il seguente file e' 'aux-functions.h' e contiene l'interfaccia per 'aux-functions.c'

    Durante il corso del progetto ho ritenuto necessario creare alcune funzioni di supporto 
    per l'esecuzione di alcune operazioni. Per tenere pero' nei vari file il codice piu' pulito
    ho deciso di raccogliere queste in un unico foglio.
  
    Le varie funzioni sono spiegate nel file 'aux-functions.c'
*/

void client_input_error();
void check_client_inputs(int argc, char* argv[]);

void supervisor_input_error();
void check_supervisor_inputs(int argc, char* argv[]);

int isNumber(char number[]);

char* integer_to_hex(int number);

int member(int n, int* array, int size);

#endif