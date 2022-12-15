#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <iostream>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "utils.h"


using namespace std;


#define GN 60
#define IP "tejo.tecnico.ulisboa.pt"
#define PORT 58000
#define BUFFERSIZE 128

char* plid;                     //Verificar necessidade
int word_len;
int max_errors;
int error;
int attempt;
char* GSip; 
char* GSport;
struct sockaddr_in addr;
char buffer[BUFFERSIZE];
char msg[BUFFERSIZE];
char receiving[BUFFERSIZE];
socklen_t addrlen;
char *l;

struct timeval t;
struct addrinfo hintsServerTCP,*resServerTCP;
struct addrinfo hintsServerUDP,*resServerUDP;
int fdServerUDP,errcode, fdServerTCP;

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
    initUDP();
    initTCP();

    while (1){
        memset(buffer, 0, BUFFERSIZE);
        fgets(buffer, BUFFERSIZE, stdin);
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
    int i;
    int num;
    char* splid = strtok(NULL, " \n");
    char f[3];
    char conf[3];
    if (strtok(NULL, " \n")!=NULL || splid==NULL){                     //Invalid input format
        cout << "Invalid input format" << endl;
        return;
    }
    if (strlen(splid)!=6 || !validPLID(splid)){ 
        cout << "Invalid ID" << endl;
        return;
    }

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "SNG %s\n", splid);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }

    addrlen=sizeof(addr);
    memset(receiving, 0, BUFFERSIZE);
    n=recvfrom(fdServerUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1){
        cout << "Unable to receive from server" << endl;
        exit(1);
    }

    printf("RECEIVING: %s", receiving);

    sscanf(receiving, "%s", f);
    if (strcmp(f, "RSG")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    sscanf(receiving, "RSG %s %d %d\n", conf, &word_len, &max_errors);
    if (strcmp(conf, "NOK")==0){
        cout << "Game already ongoing" << endl;
        exit(1); 
    }
    else if (strcmp(conf, "OK")!=0){
        cout << "Unexpected response" << endl;
        exit(1); 
    }
    
    attempt = 0;

    l = (char *)calloc((2*word_len+1),sizeof(char));
    if (l == NULL){
        perror("Error: ");
        exit(1);
    }
    l[0] = '_';
    for (i=1; i < word_len; i++){
        l[2*i-1] = ' ';
        l[2*i] = '_';
    }
    l[2*i]='\0';
    printf("New game started (max %d errors): %s\n", max_errors, l);
    plid = create_string(splid);

}

void play(){    //no server se for letra repetida 
    ssize_t n;
    int num;
    int hits;
    char* f;
    char* status;
    char* value;
    char *letter = strtok(NULL, " \n");
    
    if (strtok(NULL, " \n")!=NULL || letter==NULL){                     //Invalid input format
        cout << "Invalid input format" << endl;
        return;
    }
    if (strlen(letter)!=1 || !validAlpha(letter, 1)){
        cout << "Invalid letter" << endl;
        return;
    }

    memset(msg, 0, BUFFERSIZE);
    attempt++;
    num = sprintf(msg, "PLG %s %s %d\n", plid, letter, attempt);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }
    
    memset(receiving, 0, BUFFERSIZE);
    n=recvfrom(fdServerUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1){
        cout << "Unable to receive from server" << endl;
        exit(1);
    }

    printf("RECEIVING: %s", receiving);

    f = strtok(receiving, " \n");;

    if (strcmp(f, "RLG")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    status = strtok(NULL, " \n");

    attempt = atoi(strtok(NULL, " \n"));

    if (strcmp(status, "WIN")==0){
        for (int e=0; e<word_len; e++){
            if (l[2*e]!='_')
                continue;
            l[2*e] = toupper(letter[0]);
        }
        printf("WELL DONE! YOU GUESSED: %s\n", l);
        return; 
    }
    else if (strcmp(status, "NOK")==0){
        printf("No,\"%s\" is not part of the word: %s\n", letter, l);
        return;
    }
    else if (strcmp(status, "OVR")==0){
        printf("No,\"%s\" is not part of the word: %s\nNO MORE TRIES... GAME OVER...\n", letter, l);
        return;
    }
    else if (strcmp(status, "OK")!=0){
        cout << "Unexpected response" << endl;
        exit(1); 
    }

    hits = atoi(strtok(NULL, " \n"));

    while (hits > 0){
        value = strtok(NULL, " \n");
        l[2*(atoi(value)-1)] = toupper(letter[0]);
        hits--;
    }
        
    /*recebe play ou pl e recebe uma letra(nota: case insensitive) do input
    
    envia por UDP "PLG plid (letra escolhida) (tentativa)\n"

    recebe por UDP "(RSG) (status) (tentativa) (número de posições em que a letra se encontra) (as posições)\n"

    printf("Yes, "(letra)" is part of the word: (estado da palavra)")
    or
    printf("No, "(letra)" is not part of the word")
    */
    //if (gameover) win();


    //verificar o estado (OK, WIN, DUP, NOK, OVR, INV, ERR)
    //Se for OK , metemos nas posições recebidas, a letra correta
    printf("Yes,\"%s\" is part of the word: %s\n", letter, l);
}

void guess(){
    ssize_t n;
    int num;
    char* f;
    char* status;
    char* word = strtok(NULL, " \n");
    if (strtok(NULL, " \n")!=NULL || word==NULL){                       //Invalid input format
        cout << "Invalid input format" << endl;
        return;
    }
    if (!validAlpha(word, strlen(word))){ //talvez  verificar tamanho da palavra adivinhada numa primeira instância                   
        cout << "Invalid word" << endl;
        return;
    }
    memset(msg, 0, BUFFERSIZE);
    attempt++;
    num = sprintf(msg, "PWG %s %s %d\n", plid, word, attempt);
    printf("SENDING: %s", msg);

    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }
    
    memset(receiving, 0, BUFFERSIZE);
    n=recvfrom(fdServerUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1){
        cout << "Unable to receive from server" << endl;
        exit(1);
    }

    printf("RECEIVING: %s", receiving);

    f = strtok(receiving, " \n");;

    if (strcmp(f, "RWG")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    status = strtok(NULL, " \n");

    attempt = atoi(strtok(NULL, " \n"));

    if (strcmp(status, "WIN")==0){
        for (int e=0; e<word_len; e++){
            l[2*e] = toupper(word[e]);
        }
        printf("WELL DONE! You guessed: %s\n", l);
        exit(1); 
    }
    else if (strcmp(status, "NOK")==0){
        cout << "Wrong guess" << endl;
        return; 
    }
    else if (strcmp(status, "OK")!=0){
        cout << "Unexpected response" << endl;
        exit(1); 
    }

    

    
    /*recebe guess ou gw e recebe uma palavra(nota: case insensitive) do input
    
    envia por UDP "PWG plid (palavra) (tentativa)\n"

    recebe por UDP "(RWG) (status) (tentativa)\n"

    printf("output")
    */


}

void hint(){
    ssize_t n;
    int num;
    memset(msg,0,BUFFERSIZE);
    num = sprintf(msg, "GHL %s\n", plid);
    printf("SENDING: %s", msg);
    n = connect(fdServerTCP, (struct sockaddr*)resServerTCP->ai_addr,resServerTCP->ai_addrlen);
    if(n==-1){
        cout << "Unable to connect from user to server" << endl;
        exit(1); 
    }

    n = write(fdServerTCP,msg,num);
    if(n==-1){
        cout << "Unable to write from user to server" << endl;
        exit(1); 
    }

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
    char dport[BUFFERSIZE];
    memset(dport, 0, BUFFERSIZE);
    sprintf(dport, "%d", GN+PORT);
    gethostname(buffer, BUFFERSIZE);
    GSip = create_string(buffer);
    GSport = create_string(dport);
    for (int e = 1; e < argc; e++) {
        if (argv[e][0] == '-'){
            if (argv[e][1] == 'n'){
                free(GSip);
                GSip = create_string(argv[e+1]);
                e++;
            }
            else if (argv[e][1] == 'p'){
                free(GSport);
                GSport = create_string(argv[e+1]);
                e++;
            }
        }
        else{
            printf("\nWrong format in: %s (input argument)\n", argv[e]);
            exit(1);
        }
    }
}
void initUDP(){
    fdServerUDP=socket(AF_INET, SOCK_DGRAM, 0);
    if (fdServerUDP==-1)
        exit(1);

    memset(&hintsServerUDP,0,sizeof hintsServerUDP);
    hintsServerUDP.ai_family=AF_INET;
    hintsServerUDP.ai_socktype=SOCK_DGRAM;

    errcode = getaddrinfo(GSip, GSport, &hintsServerUDP, &resServerUDP);
    if (errcode!=0)
        exit(1);

    t.tv_sec = 4;
    t.tv_usec = 0;

    if(setsockopt(fdServerUDP, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t))!=0){
        perror("setsockopt");
        exit(1);
    }

}

void initTCP(){
    fdServerTCP=socket(AF_INET,SOCK_STREAM,0);
    if (fdServerTCP==-1) exit(1); //error

    memset(&hintsServerTCP,0,sizeof hintsServerTCP);
    hintsServerTCP.ai_family=AF_INET;
    hintsServerTCP.ai_socktype=SOCK_DGRAM;

    errcode= getaddrinfo(GSip, GSport, &hintsServerTCP, &resServerTCP);
    if(errcode!=0)/*error*/
        exit(1);
}

bool validPLID(char *string)
{
    size_t a = strspn(string, "0123456789");
    if (a==strlen(string))
        return true;
    return false;
}

bool validAlpha(char *string, size_t n)
{
    size_t a = strspn(string, "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz");
    if (a!=n){
        return false;
    }
    return true;
}

char* create_string(char* p){
    char* string = (char*)malloc((strlen(p)+1)*sizeof(char));
    if (string == NULL){
        perror("Error: ");
        exit(1);
    }
    strcpy(string, p);
    string[strlen(p)+1]='\0';
    return string;
}