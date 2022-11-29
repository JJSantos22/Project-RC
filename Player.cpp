#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#define GN 60
#define PORT 58000

int plid;
int error;
int attempt;

void initUDP();
void initTCP();

void readverifyinput(char *f);

int main(){
    char *f;
    while (1){
        readverifyinput(f);
        switch (*f){
            case ('start' or 'sg'):
                start();
                break;
            case ('play' or 'pl'):
                play();
                break;
        }
    }
    
}

void start(){
    char *splid;
    sscanf(splid, "%s", stdin);
    if (strlen(splid)!=6){
        /*gerar erro*/
    }
    initUDP();
    

    
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

    /*cona */
    
    envia por UDP uma mensagem
    
    fecha as conexões TCP*/
}

void readverifyinput(char *f){
    if (sscanf(f, "%s", stdin)!=1){
        /*gerar erro*/
    }
};