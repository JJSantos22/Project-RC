#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <iostream>

using namespace std;

#include "utils.h"

#define GN 60
#define PORT "58011"

char* plid;  //Verificar necessidade
int error;
int attempt;
char* GSip; 
char* GSport;
struct sockaddr_in addr;
char buffer[128];
socklen_t addrlen;


struct addrinfo hints,*res;
int fd,errcode;

void readStartingInput(int argc, char* argv[]);
void initUDP();
void initTCP();
void start();
void play();
void guess();
void hint();
void state();
void quit();
void exit();

int main(int argc, char* argv[]){
    char *f;
    readStartingInput(argc, argv);
    //initUDP();
    //initTCP();

    while (1){
        memset(buffer, 0, 128);
        fgets(buffer, 128, stdin);
        f = strtok(buffer, " \n"); //tem de vir de fgets do stdin
        if (!f){
            continue;
        }
        else if (strcmp(f,"start") == 0|| strcmp(f,"sg") == 0){
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
            cout << "Wrong input format" << endl;
            fflush(stdin);
        }
    }
}

void start(){
    ssize_t n;
    int num, val;
    char msg[11];
    char* splid = strtok(NULL, " \n");
    if (strtok(NULL, " \n")!=NULL || splid==NULL){                     //Invalid input format
        cout << "Invalid input format" << endl;
        return;
    }
    if (strlen(splid)!=6 || !validPLID(splid)){ 
        cout << "Invalid ID" << endl;
        return;
    }
    num = sprintf(msg, "SNG %s\n", splid);
    printf("sending: %s\n", msg);
    //n = sendto(fd, msg, num, 0, (struct sockaddr*)res->ai_addr, res->ai_addrlen);
    /* if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    } */


    
    attempt = 0;

    /*recebe start ou sg e recebe um plid do input

    envia por UDP "SNG plid\n"(6 caracteres para plid)

    recebe por UDP "(RSG) (status) (nºde letras da palavra) (número máximo de erros q se pode dar)\n"

    printf("New game started (max errors) errors: (nºde letras com underscores)");

    */
    val=6;
    char* l = (char *)malloc(val*sizeof(char));
    for (int i=0; i < val; i++)
        l[i] = '_';
    printf("New game started (max errors) errors: %s\n",l);
    plid = splid;
    free(l);
    return;

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

void readStartingInput(int argc, char *argv[]){

    for (int e = 1; e < argc; e++) {
        if (argv[e][0] == '-'){
            if (argv[e][1] == 'n'){
                GSip = create_string(argv[e+1]);
                e++;
            }
            else if (argv[e][1] == 'p'){
                GSport = create_string(argv[e+1]);
                e++;
            }
        }
        else
            printf("\nWrong format in: %s (input argument)\n", argv[e]);
    }
    //if (GSip == NULL){
        //GSip=create_string(getaddrinfo(NULL, NULL, NULL, NULL)); //rever se calhar usar gethostname
    //    GSip = NULL;
    //}
    //if (GSport == NULL)
        //GSport=create_string(getaddrinfo(NULL, NULL, NULL, NULL)); //rever 
     //   GSport = NULL;
    

}
void initUDP(){
    fd=socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1)
        exit(1);


    memset((void *)&hint,0,sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    if (errcode!=0)
        exit(1);

}

void initTCP(){

}

bool validPLID(char *string)
{
    size_t a = strspn(string, "0123456789");
    if (a==strlen(string))
        return true;
    return false;
}

char* create_string(char* p){
    char* string = (char*)malloc((strlen(p)+1)*sizeof(char));
    if (string == NULL){
        perror("Error: ");
        exit(1);
    }
    strcpy(string, p);
    return string;
}