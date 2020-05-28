/*
    Matteo Montalbetti
    MAT. 559881
    Corso B
    "OUT-OF-BAND SIGNALING"
*/

#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>
#include<string.h>

#include"aux-functions.h"

/*
    Ho deciso di creare tre file esterni per raccogliere 
    alcune funzioni che ho definito, cosi da avere un codice piu pulito 
    e una miglior organizzazione.
    La spiegazione di ognuno si trova nei rispettivi file '.c' e '.h'

    Il seguente file e' 'aux-functions.c' e implementa (e quindi #include) l'interfaccia 'aux-functions.h'

    Durante il corso del progetto ho ritenuto necessario creare alcune funzioni di supporto 
    per l'esecuzione di alcune operazioni. Per tenere pero' nei vari file il codice piu' pulito
    ho deciso di raccogliere queste in un unico foglio.  
*/

/*
    La funzione 'client_input_error' viene eseguita, in caso di errore riguardante l'input
    con il quale e' stato lanciato il client, all'interno della funzione 'check_client_inputs'.

    Cio' che fa sono delle semplici stampe informative, che suggeriscono all'utente alcune
    linee guida per l'inserimento di un input corretto.
*/
void client_input_error() {
    printf("Error -> Bad usage.\n");
    printf("Usage: ./client.o <p> <k> <w> -- where (1 <= p < k) and (w > 3p)\n");
    exit(EXIT_FAILURE);
}

/*
    @param argc --> (int) il numero di argomenti passati in input;
    @param argv --> (char*) l'array di argomenti passati in input;

    La funzione 'check_client_inputs' prende come parametri proprio cio' che viene passato al main,
    ovvero l'input a riga di comando.

    Si procede al controllo di questo, verificando:

        - il numero degli argomenti inseriti --> devono essere 4: il nome del programma, p, k e w;

        - p, k e w sono effettivamente dei numeri --> devono esserlo, altrimenti non possono essere
                                                      convertiti in interi e tanto meno utilizzati;
                                                      Per fare cio' si utilizza 'isNumber', un'altra 
                                                      funzione ausiliaria.

        - si convertono le tre stringhe in numeri, e si controlla che rispettino
          i vincoli dati dal progetto
                                        
    Se uno di questi punti dovesse trovare un errore, viene eseguita la funzione
    'client_input_error', la quale si occupa di notificare l'errore e terminare l'esecuzione 
    di 'client'.        
*/
void check_client_inputs(int argc, char* argv[]) {
    //Controllo sul numero di argomenti passati
    if(argc != 4) {
        client_input_error();
    }
    //Controllo l'eventualità di argomenti che non siano numeri
    for (int i = 1; i < 4; i++)
    {
        if(!isNumber(argv[i])) client_input_error();
    }
    //Controllo i vincoli su p e q
    int p, k, w;
    p = strtol(argv[1], NULL, 10);
    k = strtol(argv[2], NULL, 10);
    w = strtol(argv[3], NULL, 10);
    if(p < 1 || p >= k || w <= (3 * p)){
        client_input_error();
    }
}

/*
    La funzione 'supervisor_input_error' viene eseguita, in caso di errore riguardante l'input
    con il quale e' stato lanciato il supervisor, all'interno della funzione 'check_supervisor_inputs'.

    Cio' che fa sono delle semplici stampe informative, che suggeriscono all'utente alcune
    linee guida per l'inserimento di un input corretto.
*/
void supervisor_input_error() {
    printf("Error -> Bad usage.\n");
    printf("Usage: ./supervisor.o <k> -- k = #server\n");
    exit(EXIT_FAILURE);
}

/*
    @param argc --> (int) il numero di argomenti passati in input;
    @param argv --> (char*) l'array di argomenti passati in input;

    La funzione 'check_supervisor_inputs' prende come parametri proprio cio' che viene passato al main,
    ovvero l'input a riga di comando.

    Si procede al controllo di questo, verificando:

        - il numero degli argomenti inseriti --> devono essere 2: il nome del programma e k;

        - k deve essere un numero --> altrimenti non potrebbe essere convertito in interno e tanto
                                      meno utilizzato;
                                      Per fare cio' si utilizza 'isNumber', un'altra funzione
                                      ausiliaria.

    Se uno di questi punti dovesse trovare un errore, viene eseguita la funzione
    'supervisor_input_error', la quale si occupa di notificare l'errore e terminare l'esecuzione 
    di 'supervisor'.        
*/
void check_supervisor_inputs(int argc, char* argv[]) {
    if(argc != 2) {
        supervisor_input_error();
    }
    if(!isNumber(argv[1])) supervisor_input_error();
}

/*
    @param number --> (char[]) un array di caratteri;
    @return (int) la funzione ritorna un intero che indica
                  se 'number' e' effettivamente un numero
                  oppure contiene anche caratteri (ad eccezione
                  del meno iniziale)
                  1 --> 'number' e' un numero
                  0 --> 'number' non e' un numero

    La funzione 'isNumber' prende come parametre un array di caratteri, ovvero una stringa.

    Cio che questa funzione fa e' controllare se la stringa e' effettivamente un numero, oppure no.
    Si scorrono tutti i caratteri dell'array e su ognuno si chiama la funzione 'isdigit'.

    NOTA BENE --> si fa un precedente controllo sul primo carattere che potrebbe essere 
    un '-' --> isdigit(-) darebbe errore, pero' noi sappiamo che il numero potrebbe essere negativo.
*/
int isNumber(char number[]) {
    int i = 0;
    //check negative value
    if(number[0] == '-') {
        i = 1;
    }
    while(number[i] != 0) {
        if(!isdigit(number[i])) {
            return 0;
        }
        i++;
    }
    return 1;
}

/*
    @param number --> (int) numero da convertire;
    @return (char*) la funzione ritorna una stringa che 
                    consiste nella rappresentazione in base
                    esadecimale di 'number';

    La funzione 'integer_to_hex' prende in input un numero e lo converte,
    restituendo una stringa, in formato esadecimale. 

    La funzione è stata presa da Internet; tuttavia, non viene utilizzata 
    all'interno del progetto! (Si utilizza invece "%x" nella printf)
*/
char* integer_to_hex(int number) {
    char* hex;
    char* final;
    int i, result, remainder;

    hex = (char*) calloc(64, sizeof(char));
    i = 0;
    result = number;

    while(result != 0){
        remainder = result % 16;
        if(remainder < 10) {
            *(hex+i) = remainder + 48;
        } else {
            *(hex+i) = remainder + 55;
        }
        result = result / 16;
        i++;
    }

    final = (char*) calloc(i + 1, sizeof(char));
    
    for (int j = i - 1; j >= 0; j--)
    {
        *(final+(i-1-j)) = hex[j];
    }

    free(hex);
    
    return final;

}


/*
    @param n --> (int) numero da trovare;
    @param array --> (int*) array di interi;
    @param size --> (int) lunghezza di 'array';
    @return (int) la funzione ritorna un'intero che
                  notifica la presenza o meno di 'n'
                  nell'array 'array';
                  1 --> l'elemento è presente nell'array
                  0 --> l'elemento non è presente nell'array

    La funzione 'member' non fa niente di più della classica member.
    
    Controlla la presenza di un certo elemento in un dato array.
*/
int member(int n, int* array, int size) {
    for (int i = 0; i < size; i++)
    {
        if(n == *(array+i)) {
            return 1;
        }
    }
    return 0;
}

