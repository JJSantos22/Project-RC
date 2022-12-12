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

using namespace std;

#define GN 60
#define PORT "58011"
#define BUFFERSIZE 128

//Global Variables
int verbose;
char *GSport;
int word_len;
int max_errors;
char buffer[BUFFERSIZE];
char receiving[BUFFERSIZE];
int attempt;
char *word; //alterar

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
char *choose_word();

int main(int argc, char *argv[]){
    readInput(argc, argv);
    initGSUDP();
    //initTCP();

    fd_set readfds;
    char op[3];

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
            printf("RECEIVED: %s", receiving);
            sscanf(receiving,"%s", op);
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
    memset(buffer, 0, BUFFERSIZE);
    word=choose_word();
    printf("Word: %s\n", word);
    word_len = strlen(word); //alterar
    max_errors = get_max_errors(word); //alterar
    if (max_errors==-1)
        exit(1);
    num = sprintf(buffer, "RSG OK %d %d\n", word_len, max_errors);
    printf("sending: %s", buffer);
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
    int hits=2; //alterar
    int pos[hits];
    memset(buffer, 0, BUFFERSIZE);
    attempt++;
    pos[0]=2; //alterar
    pos[1]=5; //alterar
    sprintf(buffer, "RLG OK %d %d", attempt, hits);
    for (int i=0; i<hits; i++){
        sprintf(buffer, "%s %d", buffer, pos[i]);
        //strcat(buffer, s);
    }
    num = sprintf(buffer, "%s\n", buffer);
    printf("sending: %s", buffer);
    addrlen=sizeof(addr);
    n = sendto(fdClientUDP, buffer, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
}

void guess(){}

void readInput(int argc, char *argv[]){
    verbose = 0;
    if (argc)
    for (int e = 1; e < argc; e++) {
        if (argv[e][0] == '-'){
            if (argv[e][1] == 'n'){
                GSport = create_string(argv[e+1]);
                e++;
            }
            else if (argv[e][1] == 'v'){
                verbose=1;
            }
        }
        else
            printf("\nWrong format in: %s (input argument)\n", argv[e]);
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
    
}

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

char* choose_word()
{
    srand(time(0));
    char* wordList[4];
    wordList[0] = create_string("minimilk");
    wordList[1] = create_string("benfica");
    wordList[2] = create_string("computador");
    wordList[3] = create_string("veado");

    int val = rand() % 4;
    char* word = wordList[val];

    for (int i=0; i<4; i++){
        if (i!=val)
            free(wordList[i]);
    }

    return word;
}

int get_max_errors(char *word){

    int max_errors;

    if (strlen(word) >= 3 && strlen(word) <= 6)
        max_errors = 7;
    else if (strlen(word) > 6 && strlen(word) <= 11 ) 
        max_errors = 8;
    else if (strlen(word) > 11 && strlen(word) <= 30 ) 
        max_errors = 9;
    else {
        printf("Invalid word\n");
        return -1;
    }

    return max_errors;
}