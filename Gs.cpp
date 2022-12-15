#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <iostream>
#include <ctype.h>
#include <filesystem>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.h"
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <fstream>
#include <cstring>
#include <bits/stdc++.h>


using namespace std;

#define GN 60
#define PORT 58000
#define BUFFERSIZE 128

//Global Variables
int verbose;
char *GSport;   //free no fim
int word_len;
int max_errors;
char buffer[BUFFERSIZE];
char receiving[BUFFERSIZE];
int attempt;
char *word;     //free no fim
char *hintpic;     //free no fim
char *plid;     //free no fim
char *wfile;    //free no fim
int lines=0;
int thits;
int errors;
ifstream wordfile;

struct timeval t;
struct addrinfo hintsClientTCP,*resClientTCP;
struct addrinfo hintsClientUDP,*resClientUDP;
int newfd,fdClientUDP,fdClientTCP, errcode;

socklen_t addrlen;
struct sockaddr_in addr;

void readInput(int argc, char *argv[]);
char* create_string(char* p);
void initGSUDP();
void initGSTCP();
void initDB();
void start();
void play();
void guess();
void hint();
int get_max_errors(char* word);
void choose_word();
int letter_in_word(char* word, char* letter, char* pos, int word_len);
void sendTCP(int fd, char* buffer, ssize_t buffer_len);

int main(int argc, char *argv[]){
    readInput(argc, argv);
    initGSUDP();
    initGSTCP();

    fd_set readfds;
    char* op;

    while (1){
        ssize_t n;
        FD_ZERO(&readfds);
        FD_SET(fdClientUDP, &readfds);
        FD_SET(fdClientTCP, &readfds);
        if (FD_ISSET(fdClientUDP, &readfds)){
            addrlen=sizeof(addr);
            memset(receiving, 0, BUFFERSIZE);
            n=recvfrom(fdClientUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
            if (n==-1)
                exit(1);
            printf("RECEIVING: %s", receiving);
            op = strtok(receiving, " \n");
            if (strcmp(op, "SNG")==0)
                start();
            else if (strcmp(op, "PLG")==0)
                play();
            else if (strcmp(op, "PWG")==0)
                guess();
        }
        else if (FD_ISSET(fdClientTCP, &readfds)){
            addrlen=sizeof(addr);
            if((newfd=accept(fdClientTCP,(struct sockaddr*)&addr,&addrlen))==-1)
                exit(1);
                //                                 verificar o hint
                /* if (strcmp(op, "GHL")==0)
                hint(); */
        }
            
    }
    

}

void start(){
    ssize_t n;
    int num;
    char status[4];
    plid = create_string(strtok(NULL, " \n"));
    
    choose_word();
    printf("W: %s\n",word);
    word_len = strlen(word); 
    thits=word_len;
    max_errors = get_max_errors(word);
    if (max_errors==-1){
        cout << "Invalid word size" << endl;
        exit(1);
    }

    strcpy(status,"OK");

    memset(buffer, 0, BUFFERSIZE);
    num = sprintf(buffer, "RSG %s %d %d\n", status, word_len, max_errors);

    printf("SENDING: %s", buffer);
    addrlen=sizeof(addr);
    n = sendto(fdClientUDP, buffer, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    attempt=0;
    errors=0;
}

void play(){
    ssize_t n;
    int num;
    char* letter;
    char* id;
    char status[4];
    char* pos = (char*)calloc((word_len*2), sizeof(char));
    if (pos == NULL){
        perror("Error: ");
        exit(1);
    }

    id = strtok(NULL, " \n");
    if (strcmp(id, plid)!=0){ //alterar para varios jogadores
        exit(1);
    }
    letter = strtok(NULL, " \n");
    if (attempt+1 == (num=atoi(strtok(NULL, " \n")))){           //rever
        attempt++;
    }
    int hits = letter_in_word(word, letter, pos, word_len); 
    thits-=hits;
    memset(buffer, 0, BUFFERSIZE);
    if (thits==0){
        strcpy(status, "WIN");
        num = sprintf(buffer, "RLG %s %d\n", status, attempt);
    }
    else if (hits==0){
        errors++;
        if (errors>=max_errors){
            strcpy(status,"OVR");
            num = sprintf(buffer, "RLG %s %d %d\n", status, attempt, hits);
        }
            
        else {
            strcpy(status,"NOK");
            num = sprintf(buffer, "RLG %s %d %d\n", status, attempt, hits);
        }
    }
    else{
        strcpy(status,"OK");
        num = sprintf(buffer, "RLG %s %d %d%s\n", status, attempt, hits, pos);
    }
        

    printf("SENDING: %s", buffer);
    
    n = sendto(fdClientUDP, buffer, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    free(pos);
}

void guess(){
    ssize_t n;
    int num;
    char* guess;
    char* id;
    char status[4];

    id = strtok(NULL, " \n");
    if (strcmp(id, plid)!=0){ //alterar para varios jogadores
        exit(1);
    }
    
    guess = strtok(NULL, " \n");
    for (size_t i=0; i<strlen(guess); i++)
        guess[i]=toupper(guess[i]);

    if(strcmp(guess, word)==0)
        strcpy(status, "WIN");
    else
        strcpy(status,"OK");

    if(attempt+1 == (num=atoi(strtok(NULL, " \n")))){           //rever
        attempt++;
    }
    
    memset(buffer, 0, BUFFERSIZE);
    num = sprintf(buffer, "RWG %s %d\n", status, attempt);
    printf("SENDING: %s", buffer);
    
    n = sendto(fdClientUDP, buffer, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }

}

void hint(){
    




    
}


void readInput(int argc, char *argv[]){     //adicionar mais verificações
    verbose = 0;
    if (argc<2 || argc>5){
        cout << "Wrong number of arguments" << endl;
        exit(1);
    }
    
    wfile = create_string(argv[1]);
    ifstream wordfile(wfile);
    string tmp;
    if (!wordfile) {
        cout << "No word file with that name was found" << endl;
        exit(1);
    }
    while (getline(wordfile, tmp))
        lines++;                            //contar número de linhas
    wordfile.close();
    
    char dport[BUFFERSIZE];
    memset(dport, 0, BUFFERSIZE);
    sprintf(dport, "%d", GN+PORT);
    GSport = create_string(dport);
    for (int e = 2; e < argc; e++) {
        if (argv[e][0] == '-'){
            if (argv[e][1] == 'p'){
                free(GSport);
                GSport = create_string(argv[e+1]);
                e++;
            }
            else if (argv[e][1] == 'v') 
                verbose=1;
        }
        else{
            printf("\nWrong format in: %s (input argument)\n", argv[e]);
            exit(1);
        }
    }
}

void initGSUDP(){
    int n;

    fdClientUDP = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket
    if(fdClientUDP == -1)
        exit(1);

    memset(&hintsClientUDP, 0, sizeof hintsClientUDP);
    hintsClientUDP.ai_family=AF_INET; // IPv4
    hintsClientUDP.ai_socktype=SOCK_DGRAM; // UDP socket
    hintsClientUDP.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL, GSport, &hintsClientUDP, &resClientUDP);
    if(errcode!=0)
        exit(1);

    n=bind(fdClientUDP,resClientUDP->ai_addr, resClientUDP->ai_addrlen);
    if(n==-1)
        exit(1);
    
}

void initGSTCP(){
    int n;

    fdClientTCP = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if(fdClientTCP == -1)
        exit(1);
    memset(&hintsClientTCP,0,sizeof hintsClientTCP);
    hintsClientTCP.ai_family=AF_INET;
    hintsClientTCP.ai_socktype=SOCK_STREAM;
    hintsClientTCP.ai_flags=AI_PASSIVE;

    errcode = getaddrinfo (NULL,GSport, &hintsClientTCP, &resClientTCP);
    if(errcode!=0) /*error*/ 
        exit(1);

    n = bind(fdClientTCP,resClientTCP->ai_addr,resClientTCP->ai_addrlen);
    if(n==-1) /*error*/ 
        exit(1);

    if(listen(fdClientTCP,5) == -1)
        exit(1);

}

void initDB(){
    DIR *dir;
    if ((dir = opendir("GAME")) == NULL)
        mkdir("GAME", 0777);
    else
        closedir(dir);

    if ((dir = opendir("SCORE")) == NULL)
        mkdir("SCORE", 0777);
    else
        closedir(dir);
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

void choose_word(){
    srand((unsigned) time(0));
    int val = rand() % lines;
    string tmp, w, h;
    int e=0;

    ifstream wordfile(wfile);   
    if (!wordfile) {            
        cout << "No word file was found" << endl;
        exit(1);
    }
    while (e<=val){            
        getline(wordfile, tmp);
        e++;
    }
    wordfile.close();
    w = tmp.substr(0, tmp.find_first_of(" "));
    h = tmp.substr(tmp.find_first_of(" "), tmp.length()-1);
    transform(w.begin(), w.end(), w.begin(), ::toupper);
    word = create_string(&w[0]);
    hintpic = create_string(&h[0]);
}

int letter_in_word(char* word, char* letter, char* pos, int word_len){
    int hits=0;
    char add[3];
    for (int i=0; i<word_len; i++){
        if (toupper(word[i]) == toupper(letter[0])){
            hits++;
            sprintf(add, " %d", i+1);
            strcat(pos, add);
        }            
    }
    return hits;
}

int get_max_errors(char *word){

    int max_errors;

    if (strlen(word) >= 3 && strlen(word) <= 6)
        max_errors = 7;
    else if (strlen(word) > 6 && strlen(word) <= 11 ) 
        max_errors = 8;
    else if (strlen(word) > 11 && strlen(word) <= 30 ) 
        max_errors = 9;
    else 
        return -1;

    return max_errors;
}

void sendTCP(int fd, char* buffer, ssize_t buffer_len){
    ssize_t nleft, nwritten;
    char* ptr;
    ptr = &buffer[0];
    nleft=buffer_len;
    while(nleft>0){
        nwritten=write(fd,ptr,nleft);
        if(nwritten<=0)/*error*/
            exit(1);
        nleft-=nwritten;
        ptr+=nwritten;
    }
}



