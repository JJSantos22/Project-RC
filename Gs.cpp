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

using namespace std;

#define GN 60
#define PORT "58000"

//Global Variables
int verbose;
char *GSport;
int word_len;
int max_errors;
char buffer[128];

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

int main(int argc, char *argv[]){
    char *f;

    readInput(argc, argv);
    initGSUDP();
    //initTCP();

    fd_set readfds;

    while (1){
        ssize_t n;
        addrlen=sizeof(addr);
        n=recvfrom(fdClientUDP, buffer, 0, (struct sockaddr*)&addr, &addrlen);
        if (n==-1)
            exit(1);
        start();
        /* FD_ZERO(&readfds);
        FD_SET(fdClientUDP, &readfds);
        //FD_SET(fdClientTCP, &readfds);
        f = strtok(buffer, " \n"); //tem de vir de fgets do stdin
        if (!f){
            continue;
        }
        if (FD_ISSET(fdClientUDP, &readfds)){
            start();
        } */
        start();
    }
    
    initGSTCP();
    initDB();

}

void start(){
    ssize_t n;
    int num;
    char msg[15];
    sprintf(msg, "RSG OK %s %s\n", word_len, max_errors);
    printf("sending: %s\n", msg);
    n = sendto(fdClientUDP, msg, num, 0, (struct sockaddr*)resClientUDP->ai_addr, resClientUDP->ai_addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
}

void readInput(int argc, char *argv[]){
    int e=1;
    verbose = 0;
    if (argc)
    for (int e = 2; e < argc; e++) {
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

    fdClientUDP = socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(fdClientUDP==-1)
        exit(1);

    memset(&hintsClientUDP,0,sizeof hintsClientUDP);
    hintsClientUDP.ai_family=AF_INET; // IPv4
    hintsClientUDP.ai_socktype=SOCK_DGRAM; // UDP socket
    hintsClientUDP.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL,PORT,&hintsClientUDP,&resClientUDP);
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
