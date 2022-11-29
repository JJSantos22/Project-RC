#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

using namespace std;

#include "utils.h"

#define GN 60
#define PORT "58000"

int plid; 
int error;
int attempt;

struct addrinfo hints,*res;
int fd,errcode;

void initUDP();
void initTCP();
void start();
void play();
void guess();
void hint();
void state();
void quit();
void exit();

int main(){
    char *f;
    fd=socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1)
        exit(1);
    memset(&hint,0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    if (errcode!=0)
        exit(1);

        
    while (1){
        readverifyinput(f);
        if (strcmp(f,"start") == 0|| strcmp(f,"sg") == 0){
            start();
        }
        else if (strcmp(f,"play") == 0 || strcmp(f,"pl") == 0){
            play();
        }
        else if (strcmp(f,"guess") == 0 || strcmp(f,"gw") == 0){
            guess();
        }
        else if (strcmp(f,"hint") == 0 || strcmp(f,"h") == 0){
            hint();
        }
        else if (strcmp(f,"state") == 0 || strcmp(f,"st") == 0){
            state();
        }
        else if (strcmp(f,"quit") == 0){
            quit();
        }
        else if (strcmp(f,"exit") == 0){
            exit();
        }
        else {
            /*gerar erro por input errado*/
        }
    }
}

void start(){
    char *splid;
    sscanf(splid, "%s", stdin);
    if (strlen(splid)!=6 && validPLID(splid)){
        /*gerar erro*/
    }
    initUDP();
    initTCP();

    
    attempt = 0;

    /*recebe start ou sg e recebe um plid do input

    envia por UDP "SNG plid\n"(6 caracteres para plid)

    recebe por UDP "(RSG) (status) (nºde letras da palavra) (número máximo de erros q se pode dar)\n"

    printf("New game started (max errors) errors: (nºde letras com underscores)");

    */
}

void play(){
    /*recebe play ou pl e recebe uma letra(nota: case insensitive) do input
    
    envia por UDP "PLG plid (letra escolhida) (tentativa)\n"

    recebe por UDP "(RSG) (status) (tentativa) (número de posições em que a letra se encontra) (as posições)\n"

    printf("Yes, "(letra)" is part of the word: (estado da palavra)")
    or
    printf("No, "(letra)" is not part of the word")
    */
}

void guess(){
    /*recebe guess ou gw e recebe uma palavra(nota: case insensitive) do input
    
    envia por UDP "PWG plid (palavra) (tentativa)\n"

    recebe por UDP "(RWG) (status) (tentativa)\n"

    printf("output")
    */
}

void hint(){
    /*recebe hint ou h do input
    
    estabelece uma conexão TCP
    
    envia "GHL (plid)\n"
    
    recebe "RHL (status) (link)\n"
    */
}

void state(){
    /*recebe state ou st do input
    
    estabelece uma conexão TCP

    envia 
    */
}

void quit(){
    /*recebe quit do input

    envia por UDP uma mensagem
    
    fecha as conexões TCP*/
}

void exit(){
    /*recebe exit do input
    
    envia por UDP uma mensagem
    
    fecha as conexões TCP*/
}

