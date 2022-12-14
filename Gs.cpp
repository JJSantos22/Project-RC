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
char *plid;     //free no fim
char *wfile;    //free no fim
int lines=0;
int thits;
ifstream wordfile;

struct addrinfo hintsClientUDP,*resClientUDP;
int fdClientUDP,errcode;

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
int get_max_errors(char* word);
void choose_word();
int letter_in_word(char* word, char* letter, char* pos, int word_len);

int main(int argc, char *argv[]){
    readInput(argc, argv);
    initGSUDP();
    //initTCP();

    fd_set readfds;
    char* op;

    while (1){
        ssize_t n;
        FD_ZERO(&readfds);
        FD_SET(fdClientUDP, &readfds);
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
    }
    
    initGSTCP();
    initDB();

}

void start(){
    ssize_t n;
    int num;
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

    memset(buffer, 0, BUFFERSIZE);
    num = sprintf(buffer, "RSG OK %d %d\n", word_len, max_errors);

    printf("SENDING: %s", buffer);
    addrlen=sizeof(addr);
    n = sendto(fdClientUDP, buffer, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    attempt=0;
}

void play(){
    ssize_t n;
    int num;
    char* letter;
    char* id;
    char status[3+1];
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
    if(attempt+1 == (num=atoi(strtok(NULL, " \n")))){           //rever
        attempt++;
    }
    int hits = letter_in_word(word, letter, pos, word_len); 
    thits-=hits;
    if(thits==0)
        strcpy(status, "WIN");
    else
        strcpy(status, "OK");

    memset(buffer, 0, BUFFERSIZE);
    num = sprintf(buffer, "RLG %s %d %d%s\n", status, attempt, hits, pos);
    printf("SENDING: %s", buffer);
    
    n = sendto(fdClientUDP, buffer, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    free(pos);
}

void guess(){}

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

void initGSTCP(){}

void initDB(){
    DIR *dir;
    if ((dir = opendir("USERS")) == NULL)
        mkdir("USERS", 0777);
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
    string tmp;
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
    tmp = tmp.substr(0, tmp.find_first_of(" "));
    word = create_string(&tmp[0]);
}

int letter_in_word(char* word, char* letter, char* pos, int word_len){
    int hits=0;
    char add[3];
    for (int i=0; i<word_len; i++){
        if (word[i] == letter[0]){
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



