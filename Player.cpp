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
#include <fstream>


using namespace std;


#define GN 60
#define IP "tejo.tecnico.ulisboa.pt"
#define PORT 58000
#define BUFFERSIZE 256

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
void connectTCP();
void writeTCP(int fd, char buffer[], ssize_t buffer_len);
void start();
void play();
void guess();
void scoreboard();
void hint();
void state();
void quit();
void exit();
void rev();

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
        else if (strcmp(f,"scoreboard") == 0){
            scoreboard();
        }
        else if (strcmp(f,"quit") == 0){
            quit();
        }
        else if (strcmp(f,"exit") == 0){
            exit();
        }
        else if (strcmp(f, "rev")==0){
            rev();
        }
        else {
            cout << "Wrong input format" << endl;
        }
        fflush(stdin);
    }
}

void start(){
    ssize_t n;
    int i;
    int num;
    char* splid = strtok(NULL, " \n");
    char f[3];
    char conf[3];

    if (plid!=NULL)
        free(plid);
    plid = create_string(splid);

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "SNG %s\n", splid);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }


    addrlen=sizeof(addr);
    memset(receiving, '\0', BUFFERSIZE);
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
        return;
        
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

}

void play(){    //no server se for letra repetida 
    ssize_t n;
    int num;
    int hits;
    char* f;
    char* status;
    char* value;
    char *letter = strtok(NULL, " \n");
    

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "PLG %s %s %d\n", plid, letter, attempt+1);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }
    
    memset(receiving, '\0', BUFFERSIZE);
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


    if (strcmp(status, "ERR")==0){
        printf("ERROR WITH COMMAND\n");
        return;
    }     
    else if (strcmp(status, "DUP")==0){
        printf("JOGADA DUPLICADA\n");
        return; 
    }

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
    else if (strcmp(status, "INV") == 0){
        cout << "Invalid attempt,..., we'll be fixing the issue" << endl;
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
    
    printf("Yes,\"%s\" is part of the word: %s\n", letter, l);
}

void guess(){
    ssize_t n;
    int num;
    char* f;
    char* status;
    char* word = strtok(NULL, " \n");
    
    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "PWG %s %s %d\n", plid, word, attempt+1);
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

    f = strtok(receiving, " \n");

    if (strcmp(f, "RWG")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    status = strtok(NULL, " \n");

    if (strcmp(status, "ERR")==0){
        printf("ERROR WITH COMMAND\n");
        return;
    } 
    else if (strcmp(status, "DUP")==0){
        printf("JOGADA DUPLICADA\n");
        return; 
    }
    
    attempt = atoi(strtok(NULL, " \n"));
    
    if (strcmp(status, "WIN")==0){
        for (int e=0; e<word_len; e++){
            l[2*e] = toupper(word[e]);
        }
        printf("WELL DONE! You guessed: %s\n", l);
    }
    else if (strcmp(status, "OVR")==0){
        printf("Wrong guess\nNO MORE TRIES... GAME OVER...\n");
        return;
    }
    else if (strcmp(status, "NOK")==0){
        cout << "Wrong guess" << endl;
        return; 
    }
    else if (strcmp(status, "INV") == 0){
        cout << "Invalid attempt,..., we'll be fixing the issue" << endl;
        return;
    }
    else if (strcmp(status, "OK")!=0){
        cout << "Unexpected response" << endl;
        exit(1); 
    }

}

void hint(){        //The Player displays the name and size of the stored file
    ssize_t n;
    int num;
    char* ptr;
    int total;
    int blank = 0; 
    int size=0;
    int offset;
    char f[4];
    char status[6];
    char fname[BUFFERSIZE];
    
    connectTCP();

    memset(msg,0,BUFFERSIZE);
    num = sprintf(msg, "GHL %s\n", plid);
 
    printf("SENDING: %s", msg);
    writeTCP(fdServerTCP, msg, num);

    memset(receiving, 0, BUFFERSIZE);
    total=0;
    ptr=&receiving[0];
    while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total)) > 0){
        ptr += n;
        total += n;
    }
    for (offset=0; offset<BUFFERSIZE && blank<4; offset++){
        if (receiving[offset]==' '){
            blank++;
        }
    }
    bzero(f, 4);
    sscanf(receiving, "%s", f); 
    if (strcmp(f, "RHL") != 0){
        cout << "Wrong return message from server to user" << endl;
        exit(1);
    }

    bzero(status, 6);
    sscanf(receiving, "RHL %s", status);
    if (strcmp(status, "NOK") == 0){
        cout << "Something went wrong." << endl; 
        return;
    }
    else if (strcmp(status,"OK") != 0){
        cout << "Wrong return messsage from server to user." << endl;
        exit(1);
    }

    bzero(fname, BUFFERSIZE);
    sscanf(receiving, "RHL OK %s %d", fname, &size);
    FILE *file = fopen(fname, "w");
    fwrite(&receiving[offset], 1, BUFFERSIZE-offset, file);

    size-=(BUFFERSIZE-offset);//descobrir se é com -1 ou não
    while (size>0){
        memset(receiving, 0, BUFFERSIZE);
        total=0;
        ptr=&receiving[0];
        while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total))>0){
            ptr += n;
            total += n;
        }
        size-=total;
        if (total==BUFFERSIZE)          //talvez modificar
            fwrite(&receiving[0], 1, total, file);
        else 
            fwrite(&receiving[0], 1, total-1, file);
    }
    
    fclose(file);
    close(fdServerTCP);
}

void state(){

    ssize_t n;
    int num;
    char* ptr;
    int total;
    int blank = 0; 
    int size=0;
    int offset;
    char f[4];
    char status[4];
    char fname[BUFFERSIZE];
    
    connectTCP();

    memset(msg,0,BUFFERSIZE);
    num = sprintf(msg, "STA %s\n", plid);
 
    printf("SENDING: %s", msg);
    writeTCP(fdServerTCP, msg, num);

    memset(receiving, 0, BUFFERSIZE);
    total=0;
    ptr=&receiving[0];
    while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total)) > 0){
        ptr += n;
        total += n;
    }
    printf("receiving: %s\n", receiving);
    for (offset=0; offset<BUFFERSIZE && blank<4; offset++){
        if (receiving[offset]==' '){
            blank++;
        }
    }
    bzero(f, 4);
    sscanf(receiving, "%s", f);
    if (strcmp(f, "RST") != 0){
        cout << " Wrong return message from server to user" << endl;
        exit(1);
    }

    bzero(status, 4);
    sscanf(receiving, "RST %s", status);

    if (strcmp(status, "NOK") == 0){
        cout << "No games(active or finished) for this player." << endl; 
        close(fdServerTCP);
        return;
    }
    else if (strcmp(status,"ACT") == 0){
        sscanf(receiving, "RST ACT %s %d", fname, &size);
    } 
    else if(strcmp(status,"FIN") == 0){
        sscanf(receiving, "RST FIN %s %d", fname, &size);
    }
    else{
        cout << "Wrong return messsage from server to user." << endl;
        exit(1);
    }

    bzero(fname, BUFFERSIZE);
    FILE *file = fopen(fname, "w");
    fwrite(&receiving[offset], 1, BUFFERSIZE-offset, file);

    size-=(BUFFERSIZE-offset-1);
    printf("%s", &receiving[offset]);
    while (size > 0){
        memset(receiving, 0, BUFFERSIZE);
        total=0;
        ptr=&receiving[0];
        while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total))>0){
            ptr += n;
            total += n;
        }
        size-=total;
        if (total==BUFFERSIZE){
            fwrite(&receiving[0], 1, total, file);
        }
        else{
            fwrite(&receiving[0], 1, total-1, file);
        }
        printf("%s", receiving);
    }
    
    fclose(file);
    close(fdServerTCP);
    /*recebe state ou st do input
    
    estabelece uma conexao TCP

    envia 
    */
}

void quit(){

    ssize_t n;
    int num;
    char f[3];
    char status[3];

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "QUT %s\n", plid);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }

    memset(receiving, '\0', BUFFERSIZE);
    n=recvfrom(fdServerUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1){
        cout << "Unable to receive from server" << endl;
        exit(1);
    }

    printf("RECEIVING: %s", receiving);

    sscanf(receiving, "%s", f);
    if (strcmp(f, "RQT")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    sscanf(receiving, "RQT %s\n", status);

    if (strcmp(status, "OK") == 0){
        cout << "You quit" << endl;
        
    }
    else if (strcmp(status, "NOK") == 0){
        cout << "There wasn't an ongoing Game" << endl;
        
    }
    else{
        cout << "ERROR" << endl;
        
    }

    /*recebe quit do input

    envia por UDP uma mensagem
    
    fecha as conexoes TCP*/
}

void scoreboard(){
    int total;
    ssize_t n;
    int num;
    char* ptr;
    char f[4];
    char status[6];
    int blank = 0; 
    int offset;
    int size;
    char fname[BUFFERSIZE];

    connectTCP();

    memset(msg,0,BUFFERSIZE);
    num = sprintf(msg, "GSB\n");
    
    writeTCP(fdServerTCP, msg, num);
    
    memset(receiving, 0, BUFFERSIZE);
    total=0;
    ptr=&receiving[0];
    while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total))>0){
        ptr += n;
        total += n;
    }
    printf("receiving: %s\n", receiving);
    for (offset=0; offset<BUFFERSIZE && blank<4; offset++){
        if (receiving[offset]=='\n'){ //empty
            blank=4;
        }
        else if (receiving[offset]==' '){
            blank++;
        }
    }      
    bzero(f, 4);
    sscanf(receiving, "%s", f); 
    if (strcmp(f, "RSB") != 0){
        cout << "Wrong return message from server to user" << endl;
        exit(1);
    }

    bzero(status, 6);
    sscanf(receiving, "RSB %s", status);
    if (strcmp(status, "EMPTY") == 0){
        cout << "The scoreboard is empty." << endl; 
        return;
    }
    else if (strcmp(status,"OK") != 0){
        cout << "Wrong return messsage from server to user." << endl;
        exit(1);
    }
    
    bzero(fname, BUFFERSIZE);
    sscanf(receiving, "RSB OK %s %d", fname, &size);
    FILE *file = fopen(fname, "w");
    fwrite(&receiving[offset], 1, BUFFERSIZE-offset, file);
    
    size-=(BUFFERSIZE-offset-1);
    printf("%s", &receiving[offset]);
    while (size>0){
        memset(receiving, 0, BUFFERSIZE);
        total=0;
        ptr=&receiving[0];
        while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total))>0){
            ptr += n;
            total += n;
        }
        size-=total;
        if (total==BUFFERSIZE){
            fwrite(&receiving[0], 1, total, file);
        }
        else {
            fwrite(&receiving[0], 1, total-1, file);
        }
        printf("%s", receiving);
    }
    
    fclose(file);
    close(fdServerTCP);
}

void exit(){

    ssize_t n;
    int num;
    char f[3];
    char status[3];

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "QUT %s\n", plid);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }

    memset(receiving, '\0', BUFFERSIZE);
    n=recvfrom(fdServerUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1){
        cout << "Unable to receive from server" << endl;
        exit(1);
    }

    printf("RECEIVING: %s", receiving);

    sscanf(receiving, "%s", f);
    if (strcmp(f, "RQT")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    sscanf(receiving, "RQT %s\n", status);

    if (strcmp(status, "OK") == 0){
        cout << "Bye Bye" << endl;
        exit(1); 
    }
    else if (strcmp(status, "NOK") == 0){
        cout << "There wasn't an ongoing Game but .. BYE" << endl;
        exit(1); 
    }
    else{
        cout << "ERROR" << endl;
        exit(1); 
    }
    /*recebe exit do input
    
    envia por UDP uma mensagem
    
    fecha as conexoes TCP*/
}

void rev(){

    int num;
    char f[3];
    ssize_t n;
    char word[30];

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "REV %s\n", plid);
    printf("SENDING: %s", msg);
    n = sendto(fdServerUDP, msg, num, 0, (struct sockaddr*)resServerUDP->ai_addr, resServerUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from user to server" << endl;
        exit(1); 
    }

    memset(receiving, '\0', BUFFERSIZE);
    n=recvfrom(fdServerUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1){
        cout << "Unable to receive from server" << endl;
        exit(1);
    }

    printf("RECEIVING: %s", receiving);

    sscanf(receiving, "%s", f);
    if (strcmp(f, "RRV")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    sscanf(receiving, "RRV %s\n", word);

    printf("GAME OVER! The word was: %s\n", word);

}


void readStartingInput(int argc, char *argv[]){
    char dport[BUFFERSIZE];
    int c1=0;
    int c2=0;
    
    for (int e = 1; e < argc; e++) {
        if (argv[e][0] == '-'){
            if (argv[e][1] == 'n' && c1==0){
                GSip = create_string(argv[e+1]);
                e++;
                c1++;
            }
            else if (argv[e][1] == 'p' && c2==0){
                GSport = create_string(argv[e+1]);
                e++;
                c2++;
            }
        }
        else{
            printf("\nWrong format in: %s (input argument)\n", argv[e]);
            exit(1);
        }
    }
    if (c1 ==0){
        gethostname(buffer, BUFFERSIZE);
        GSip = create_string(buffer);
    }
    if (c2 ==0){
        memset(dport, 0, BUFFERSIZE);
        sprintf(dport, "%d", GN+PORT);
        GSport = create_string(dport);
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

    t.tv_sec = 5;
    t.tv_usec = 0;

    if(setsockopt(fdServerUDP, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t))!=0){
        perror("setsockopt");
        exit(1);
    }

}

void initTCP(){
    fdServerTCP=socket(AF_INET,SOCK_STREAM,0);
    if (fdServerTCP==-1) 
        exit(1); //error     
}

void connectTCP(){
    ssize_t n;

    memset(&hintsServerTCP,0,sizeof hintsServerTCP);
    hintsServerTCP.ai_family=AF_INET;
    hintsServerTCP.ai_socktype=SOCK_DGRAM;

    errcode = getaddrinfo(GSip, GSport, &hintsServerTCP, &resServerTCP);
    if(errcode!=0)/*error*/
        exit(1);

    n = connect(fdServerTCP, resServerTCP->ai_addr,resServerTCP->ai_addrlen);
    if(n==-1){
        cout << "Unable to connect from user to server" << endl;
        exit(1); 
    }
    t.tv_sec=20;
    t.tv_usec=0;
    if (setsockopt(fdServerTCP, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t))!=0)
        exit(1);
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

void writeTCP(int fd, char buffer[], ssize_t buffer_len){
    ssize_t nleft, nwritten;
    char* ptr = &buffer[0];
    nleft=buffer_len;
    while(nleft>0){
        nwritten=write(fd,ptr,nleft);
        if(nwritten<=0)/*error*/
            exit(1);
        nleft-=nwritten;
        ptr+=nwritten;
    }
}