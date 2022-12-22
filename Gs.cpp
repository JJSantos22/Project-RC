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
#include <algorithm>
#include <map>


using namespace std;

#define GN 60
#define PORT 58000
#define BUFFERSIZE 128

//Global Variables
int verbose;
char *GSport;   //free no fim
char buffer[BUFFERSIZE];
char receiving[BUFFERSIZE];
char sending[BUFFERSIZE];
char *wfile;    //free no fim
char *aux_word; 
char *aux_hint; 
int lines=0;
int countword=0;
ifstream wordfile;

struct timeval t;
struct addrinfo hintsClientTCP,*resClientTCP;
struct addrinfo hintsClientUDP,*resClientUDP;
int newfd,fdClientUDP,fdClientTCP, errcode;

socklen_t addrlen;
struct sockaddr_in addr;

void readInput(int argc, char *argv[]);
void TCP_operations(int newfd);
char* create_string(char* p);
void initGSUDP();
void initGSTCP();
void initDB();
void connectTCP();
void create_ongoing_game_file(char *plid, char *word, char *hint);
void update_file(char *plid, char *add);
void start();
void play();
void guess();
void state();
void hint();
void quit_exit();
void rev();
void scoreboard();
bool validPLID(char *string);
bool validAlpha(char *string, size_t n);
int get_max_errors(char* word);
void choose_word();
int letter_in_word(char* word, char letter, char* pos, int word_len);
void writeTCP(int fd, char buffer[], ssize_t buffer_len);
bool duplicateplay(char* plid, char *f);
bool compare_word(char* guess, char* word);
bool has_active_game(char* plid);
void create_finished_game_file(char* plid, char code);
void create_score_file(char *plid, char *time_str, char* dfilename);
void create_active_state_file(char *plid, char* fname);
void create_finished_state_file(char *plid, char* filename);
char* get_state_content_string(char* filename);
char* get_word(char* filename);
int get_thits(char* filename, char* word);
char* get_hint(char* filename);
int get_attempt(char* filename);
int get_errors(char* filename, char* word);
char* get_termination(char* fname);
bool islastplay(char* plid,char* move);
char* get_time_string();
int FindLastGame(char *plid, char *filename);

int main(int argc, char *argv[]){
    readInput(argc, argv);
    initDB();
    initGSUDP();
    initGSTCP();
    connectTCP();

    fd_set readfds;
    char* op;
    int val, ver, m, choice;

    while (1){
        ssize_t n;
        
        FD_ZERO(&readfds);
        FD_SET(fdClientUDP, &readfds);
        FD_SET(fdClientTCP, &readfds);
        m=max(fdClientTCP, fdClientUDP);
        choice=select(m+1, &readfds, (fd_set*) NULL, (fd_set*) NULL, (timeval*) NULL);

        if (choice <= 0)
            exit(1);

        if (FD_ISSET(fdClientUDP, &readfds)){
            addrlen=sizeof(addr);
            memset(receiving, 0, BUFFERSIZE);
            n=recvfrom(fdClientUDP, receiving, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen);
            if (n==-1)
                exit(1);
            printf("RECEIVING UDP: %s", receiving);
            op = strtok(receiving, " \n");
            if (strcmp(op, "SNG")==0)
                start();
            else if (strcmp(op, "PLG")==0)
                play();
            else if (strcmp(op, "PWG")==0)
                guess();
            else if (strcmp(op, "QUT")==0)
                quit_exit();
            else if (strcmp(op, "REV")==0)
                rev();
        }
        else if (FD_ISSET(fdClientTCP, &readfds)){
            addrlen=sizeof(addr);
            if((newfd=accept(fdClientTCP,(struct sockaddr*)&addr,&addrlen))==-1)
                exit(1);
            if ((val=fork())<0){
                cout << "Creation of a child process was unsuccessful" << endl;
                exit(1);
            }
            else if(val==0){
                cout << "Creation of a child process was successful" << endl;
                TCP_operations(newfd);
            }
            ver = close(newfd);
            if (ver == -1)
                exit(1);
        }    
    }
}

void start(){
    ssize_t n;
    int num;
    char status[4];
    int max_errors;
    int attempt;
    
    char* plid = create_string(strtok(NULL, " \n"));
    
    memset(sending, 0, BUFFERSIZE);
    if (strtok(NULL, " \n")!=NULL || plid==NULL || strlen(plid)!=6 || !validPLID(plid)){                     //Invalid input format
        strcpy(sending, "RSG ERR\n");
        n = sendto(fdClientUDP, sending, 8, 0, (struct sockaddr *) &addr, addrlen);
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }

    if (!has_active_game(plid)){
        choose_word();
        printf("W: %s\n",aux_word);
        max_errors = get_max_errors(aux_word);
        if (max_errors==-1){
            cout << "Invalid word size" << endl;
            exit(1);
        }
        strcpy(status,"OK");
        num = sprintf(sending, "RSG %s %ld %d\n", status, strlen(aux_word), max_errors);
        create_ongoing_game_file(plid, aux_word, aux_hint);
        free(aux_hint);
        free(aux_word);
    }
    else{
        char filename[BUFFERSIZE];
        sprintf(filename, "./GAMES/GAME_%s.txt", plid);
        attempt=get_attempt(filename);
        if (attempt>0){
            strcpy(status,"NOK");
            num = sprintf(sending, "RSG %s\n", status);
        }
        else{
            aux_word=(char*) calloc(BUFFERSIZE, sizeof(char));          
            if (aux_word==NULL){
                exit(1);
            }
            aux_word=get_word(filename);
            max_errors=get_max_errors(aux_word);
            strcpy(status,"OK");
            num = sprintf(sending, "RSG %s %ld %d\n", status, strlen(aux_word), max_errors);
            free(aux_word);
        }
    }

    printf("SENDING: %s", sending);
    addrlen=sizeof(addr);
    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    free(plid);
}

void play(){
    ssize_t n;
    int num;
    char* letter;
    char* id;
    char status[4];
    char move[BUFFERSIZE];
    int attempt;
    int thits;
    int errors;
    int val;
    int max_errors;

    id = strtok(NULL, " \n");
    if (id==NULL || strlen(id)!=6 || !validPLID(id) || !has_active_game(id)){                     //Invalid input format
        strcpy(sending, "RLG ERR\n");
        n = sendto(fdClientUDP, sending, 8, 0, (struct sockaddr *) &addr, addrlen);
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }

    letter = strtok(NULL, " \n");

    val = atoi(strtok(NULL, " \n"));

    memset(sending, 0, BUFFERSIZE);
    if (strtok(NULL, " \n")!=NULL || letter==NULL || strlen(letter)!=1 || !validAlpha(letter, 1) || !has_active_game(id)){
        strcpy(sending, "RLG ERR\n");
        n = sendto(fdClientUDP, sending, 8, 0, (struct sockaddr *) &addr, addrlen);
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }

    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", id);
    char *word = get_word(filename);
    int phits=get_thits(filename, word);
    attempt=get_attempt(filename);
    errors=get_errors(filename, word);
    thits=strlen(word);
    thits-=phits;
    max_errors=get_max_errors(word);

    char* pos = (char*)calloc((strlen(word)*2), sizeof(char));
    if (pos == NULL){
        perror("Error: ");
        exit(1);
    }

    sprintf(move, "T %c", toupper(letter[0]));
    if (val==attempt){
        if(islastplay(id, move)){
            strcpy(status,"OK");
            int hits = letter_in_word(word, letter[0], pos, strlen(word)); 
            num = sprintf(sending, "RLG %s %d %d%s\n", status, attempt, hits, pos);
        }
        else{
            strcpy(status,"INV");
            num = sprintf(sending, "RLG %s %d\n", status, attempt);
        }
    }
    else if (val!=attempt+1){
        strcpy(status,"INV");
        num = sprintf(sending, "RLG %s %d\n", status, attempt);
    }
    else if (duplicateplay(id, move)){
        strcpy(status, "DUP");
        num = sprintf(sending, "RLG %s %d\n", status, attempt);
    }
    else{
        int hits = letter_in_word(word, letter[0], pos, strlen(word)); 
        thits-=hits;
        attempt++;
        if (thits==0){
            strcpy(status, "WIN");
            num = sprintf(sending, "RLG %s %d\n", status, attempt);
        }
        else if (hits==0){
            errors++;
            if (errors>=max_errors){
                strcpy(status,"OVR");
                num = sprintf(sending, "RLG %s %d %d\n", status, attempt, hits);
            }
                
            else {
                strcpy(status,"NOK");
                num = sprintf(sending, "RLG %s %d %d\n", status, attempt, hits);
            }
        }
        else{
            strcpy(status,"OK");
            num = sprintf(sending, "RLG %s %d %d%s\n", status, attempt, hits, pos);
        }
        sprintf(move, "T %c\n", letter[0]);
        update_file(id, move);

    }
    
    printf("SENDING: %s", sending);

    if (strcmp(status, "WIN")==0) {
        create_finished_game_file(id, 'W');
    }
    else if (strcmp(status, "OVR")==0) {
        create_finished_game_file(id, 'F');
    }
    
    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    free(pos);
    free(word);
}

void guess(){           
    ssize_t n;
    int num;
    char* guess;
    char* id;
    char status[4];
    int val;
    int attempt;
    int errors;
    int max_errors;
    char move[BUFFERSIZE];
    char *hint=(char*) calloc(BUFFERSIZE, sizeof(char));         
    if (hint==NULL){
        exit(1);
    }
    id = strtok(NULL, " \n");
    if (id==NULL || strlen(id)!=6 || !validPLID(id) || !has_active_game(id)){                     //Invalid input format
        strcpy(sending, "RWG ERR\n");
        n = sendto(fdClientUDP, sending, 8, 0, (struct sockaddr *) &addr, addrlen);
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }
    guess = strtok(NULL, " \n");
    val=atoi(strtok(NULL, " \n"));           

    memset(sending, 0, BUFFERSIZE);
    if (strtok(NULL, " \n")!=NULL || guess==NULL || !validAlpha(guess, strlen(guess)) || !has_active_game(id)){    
        strcpy(sending, "RWG ERR\n");
        n = sendto(fdClientUDP, sending, 8, 0, (struct sockaddr *) &addr, addrlen);
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }

    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", id);
    char *word=get_word(filename);
    max_errors=get_max_errors(word);
    attempt=get_attempt(filename);
    errors=get_errors(filename, word);

    sprintf(move, "G %s", guess);
    if (val==attempt){
        if (islastplay(id, move)){
            if (!compare_word(guess, word))
                strcpy(status, "NOK");
        }
        else
            strcpy(status,"INV");
    }
    else if (val!=attempt+1)
        strcpy(status,"INV");
    else if (duplicateplay(id, move)){
        strcpy(status, "DUP");
        num = sprintf(sending, "RLG %s %d\n", status, attempt);
    }
    else{
        attempt++;
        if (compare_word(guess, word)){
            strcpy(status, "WIN");  
        }
        else{
            errors++;
            if (errors>=max_errors){
                strcpy(status,"OVR");
            }
                
            else {
                strcpy(status,"NOK");
            }
        }
        sprintf(move, "G %s\n", guess);
        update_file(id, move);
    }

    if (strcmp(status, "WIN")==0) {
        create_finished_game_file(id, 'W');
    }
    else if (strcmp(status, "OVR")==0) {
        create_finished_game_file(id, 'F');
    }
    
    num = sprintf(sending, "RWG %s %d\n", status, attempt);
    printf("SENDING: %s", sending);

    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }

    free(word);
}

void hint(){
    int num;
    char* id;
    long int fsize;
    size_t n;

    id = strtok(NULL, " \n");
    if (id==NULL || strlen(id)!=6 || !validPLID(id) || !has_active_game(id)){                     //Invalid id
        num=sprintf(sending, "RHL ERR\n");
        writeTCP(fdClientTCP, sending, num);
        return;
    }

    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", id);
    char* fname = get_hint(filename);
    memset(sending, 0, BUFFERSIZE);
    FILE *file = fopen(fname, "r");      
    if (file == NULL){
        num = sprintf(sending, "RHL NOK\n");
        writeTCP(fdClientTCP, sending, num);
        return;
    }
    fseek(file, 0L, SEEK_END);
    fsize = ftell(file);
        
    num = sprintf(sending, "RHL OK %s %ld ", fname, fsize);
    free(fname);
    writeTCP(fdClientTCP, sending, num);
    fseek(file, 0L, SEEK_SET);
    while (fsize>0){
        memset(sending, 0, BUFFERSIZE);
        n = fread(sending, 1, BUFFERSIZE, file);
        fsize-=n;
        writeTCP(fdClientTCP, sending, n);
    }
    memset(&sending[0], '\n', 1);
    writeTCP(fdClientTCP, sending, 1);
    fclose(file);
}


void quit_exit(){

    char status[4];
    ssize_t n;
    char* id;
    int num;

    id = strtok(NULL, " \n");
    memset(sending, 0, BUFFERSIZE);
    if (strtok(NULL, " \n")!=NULL){
        strcpy(sending, "RQT ERR\n");
        n = sendto(fdClientUDP, sending, 8, 0, (struct sockaddr *) &addr, addrlen);
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }
    
    if (has_active_game(id)){
        strcpy(status, "OK");
        create_finished_game_file(id, 'Q');
    }
    else{
        strcpy(status, "NOK");
    }  
    num = sprintf(sending, "RQT %s\n", status);
    printf("SENDING: %s", buffer);
    
    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    } 
}

void rev(){

    char* id;
    int num;
    ssize_t n;

    id = strtok(NULL, " \n");
    memset(sending, 0, BUFFERSIZE);
    if (strtok(NULL, " \n")!=NULL || (!has_active_game(id))){
        n = sendto(fdClientUDP, sending, strlen(sending), 0, (struct sockaddr *) &addr, addrlen); //VER
        if (n==-1){
            cout << "Unable to send from server to user" << endl;
            exit(1); 
        }
        return;
    }

    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", id);
    char* word = get_word(filename);

    num = sprintf(sending, "RRV %s\n", word);
    printf("SENDING: %s", buffer);

    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    } 

    free(word);
    create_finished_game_file(id, 'F');
}

void scoreboard(){

    int num;
    long int fsize;
    size_t n;
    
    char* fname = "TOPSCORES_0015863.txt";         //alterar depois
    memset(sending, 0, BUFFERSIZE);

    FILE *file = fopen(fname, "r");             //Alterar no fim
    if (file == NULL){
        num = sprintf(sending, "RSB EMPTY\n");                    //alterar
        writeTCP(fdClientTCP, sending, num);
        return;
    }

    fseek(file, 0L, SEEK_END);
    fsize = ftell(file);

    num = sprintf(sending, "RSB OK %s %ld ", fname, fsize);    
    writeTCP(fdClientTCP, sending, num);
    fseek(file, 0L, SEEK_SET);
    while (fsize>0){
        memset(sending, 0, BUFFERSIZE);
        n = fread(sending, 1, BUFFERSIZE, file);
        fsize-=n;
        writeTCP(fdClientTCP, sending, n);
    }
    memset(&sending[0], '\n', 1);
    writeTCP(fdClientTCP, sending, 1);
    fclose(file);
}

void state(){

    int num;
    long int fsize;
    size_t n;
                                   
    char* id = strtok(NULL, " \n"); 

    memset(sending, 0, BUFFERSIZE);
    if (id==NULL || strlen(id)!=6 || !validPLID(id)){                     //Invalid id
        num=sprintf(sending, "RHL ERR\n");
        writeTCP(fdClientTCP, sending, num);
        return;
    }                         
    char* fname = (char*) calloc(BUFFERSIZE, sizeof(char));                //utilizar para enviar nome de ficheiro
    if (fname==NULL)
        exit(1);

    if (has_active_game(id)){
        num = sprintf(sending, "RST ACT ");   
        writeTCP(fdClientTCP, sending, num);
        create_active_state_file(id, fname);
    }
    else if(FindLastGame(id, fname)==1){
        num = sprintf(sending, "RST FIN ");                  
        writeTCP(fdClientTCP, sending, num);
        create_finished_state_file(id, fname);
    }
    else{
        num = sprintf(sending, "RST NOK\n");                  
        writeTCP(fdClientTCP, sending, num);
        return;
    }

    FILE *file = fopen(fname, "r");             
    if (file == NULL){
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    fsize = ftell(file);

    num = sprintf(sending, "%s %ld ", fname, fsize);    
    writeTCP(fdClientTCP, sending, num);
    fseek(file, 0L, SEEK_SET);
    while (fsize>0){
        memset(sending, 0, BUFFERSIZE);
        n = fread(sending, 1, BUFFERSIZE, file);
        fsize-=n;
        writeTCP(fdClientTCP, sending, n);
    }
    memset(&sending[0], '\n', 1);
    writeTCP(fdClientTCP, sending, 1);
    fclose(file);
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
        lines++;                           
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

    fdClientTCP = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if(fdClientTCP == -1)
        exit(1);
}

void connectTCP(){
    int n;

    memset(&hintsClientTCP,0,sizeof hintsClientTCP);          
    hintsClientTCP.ai_family=AF_INET;
    hintsClientTCP.ai_socktype=SOCK_STREAM;
    hintsClientTCP.ai_flags=AI_PASSIVE;

    errcode = getaddrinfo (NULL, GSport, &hintsClientTCP, &resClientTCP);
    if(errcode!=0) /*error*/ 
        exit(1);

    n = bind(fdClientTCP, resClientTCP->ai_addr, resClientTCP->ai_addrlen);
    if(n==-1) /*error*/ 
        exit(1);

    if(listen(fdClientTCP,5) == -1)
        exit(1);
}

void initDB(){
    DIR *dir;
    if ((dir = opendir("GAMES")) == NULL)
        mkdir("GAMES", 0777);
    else
        closedir(dir);

    if ((dir = opendir("SCORES")) == NULL)
        mkdir("SCORES", 0777);
    else
        closedir(dir);
}

void create_ongoing_game_file(char *plid, char *word, char *hint){
    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", plid);
    ofstream outfile (filename);
    outfile << word << " " << hint << endl;
    outfile.close();
}

void create_active_state_file(char *plid, char* fname){
    char filename[BUFFERSIZE];
    sprintf(filename, "STATE_%s.txt", plid);

    sprintf(fname, "./GAMES/GAME_%s.txt", plid);  

    ofstream file (filename);      
    char* content = get_state_content_string(fname);
    file << "\t" << "Active game found for player " << plid << endl << "\t" << content << endl;
    free(content);
    file.close();
}

void create_finished_state_file(char *plid, char* filename){
    char fname[BUFFERSIZE];
    sprintf(fname, "STATE_%s.txt", plid); 

    ofstream file (fname);      
    char* content = get_state_content_string(filename);
    char*word=get_word(filename);
    char*hint=get_hint(filename);
    char*termination=get_termination(filename);
    file << "\t" << "Last finalized game for player " << plid << endl << "\tWord: " << word << "; Hint file: " << hint << endl << content << "\n\tTermination: " << termination;
    free(content);
    free(word);
    free(hint);
    free(termination);
    file.close();
}

char* get_termination(char* fname){
    char c[1];
    sscanf(fname, "%*s_%*s_%c.txt", c);
    char* termination = (char*) calloc(6, sizeof(char));
    if (termination==NULL)
        exit(1);
    if (c[1]=='W')
        sprintf(termination, "WIN\n");
    else if (c[1]=='F')
        sprintf(termination, "FAIL\n");
    else 
        sprintf(termination, "QUIT\n");
    return termination;
}


char* get_state_content_string(char *filename){
    int attempt;
    char* ret=(char*) calloc(BUFFERSIZE*2, sizeof(char));
    if (ret==NULL)
        exit(1);

    char* word = get_word(filename);
    attempt=get_attempt(filename);
    if (attempt==0){
        char under[strlen(word)+1];
        memset(under, '-', strlen(word));
        sprintf(ret, "Game started - no transactions found\n\tSolved so far: %s\n", under);     //atenção barra n
    }
    else {
        int n;
        char w[BUFFERSIZE];
        char* ptr=&ret[0];
        char under[strlen(word)+1];
        bzero(under, strlen(word)+1);
        memset(under, '-', strlen(word));
        n = sprintf(ret, "\t--- Transactions found: %d ---\n", attempt);
        ptr+=n;
        ifstream file(filename);
        int hit;
        string tmp;
        getline(file, tmp);
        while (getline(file, tmp)){
            if (tmp.c_str()[0] == 'T' ){
                n=sprintf(ptr, "\tLetter trial: %c - ", tmp.c_str()[2]);
                ptr+=n;
                hit=0;
                for (size_t i=0; i<strlen(word); i++){
                    if (toupper(word[i]) == toupper(tmp.c_str()[2])){
                        under[i]=tmp.c_str()[2];    
                        hit++;    
                    }
                }
                if (hit>0){
                    n=sprintf(ptr, "TRUE\n");
                    ptr+=n;
                }   
                else {
                    n=sprintf(ptr, "FALSE\n");
                    ptr+=n;
                }
            }
            else{
                bzero(w, BUFFERSIZE);
                sscanf(tmp.c_str(), "%*c %s", w);
                if (strcmp(w, word)==0){
                    strcpy(under, word);
                }
                n=sprintf(ptr, "\tWord guessed: %s\n", w);
                ptr+=n;
            }
        }
        sprintf(ptr, "\tSolved so far: %s", under);
        file.close();
    }
    return ret;
}

char* get_time_string(){
    time_t now = time(0);
    tm *ltm = localtime(&now);

    int year = 1900 + ltm->tm_year;
    int month = 1 + ltm->tm_mon;
    int day = ltm->tm_mday;
    int hour = ltm->tm_hour;
    int min = ltm->tm_min;
    int sec = ltm->tm_sec;

    char* time_str=(char*) calloc(16, sizeof(char));

    sprintf(time_str, "%d%d%d_%d%d%d", year, month, day, hour, min, sec);

    return time_str;
}

void create_score_file(char *plid, char *time_str, char* dfilename) {
    int attempt, errors;
    
    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", plid);
    char* word = get_word(filename);
    attempt = get_attempt(filename);
    errors = get_errors(filename, word);

    int succ = (attempt-errors)*100;
    int score = succ/attempt;

    int n1=score%10;
    int d1=score/10;
    int n2=d1%10;
    int d2=d1/10;
    int n3=d2%10;

    char sscore[4];
    sprintf(sscore, "%d%d%d", n3,n2,n1);

    sprintf(filename, "./SCORES/%s_%s_%s.txt", sscore, plid, time_str);

    free(time_str);

    char buffer[BUFFERSIZE];
    sprintf(buffer, "%s %s %s %d %d", sscore, plid, word, attempt-errors, attempt);

    ofstream outfile (filename);
    outfile << buffer;
    outfile.close();

    char command[BUFFERSIZE];
    sprintf(command, "rm %s", dfilename);
    system(command);
    free(word);
}

void create_finished_game_file(char* plid, char code){ //ADD CHAR CODE 
    char newfilename[50];
    DIR *dir;
    
    char filename[50];
    sprintf(filename, "./GAMES/GAME_%s.txt", plid);

    char* time = get_time_string(); 
   
    sprintf(newfilename, "./GAMES/%s/%s_%c.txt", plid, time, code);

    char directory[50];
    sprintf(directory,"./GAMES/%s", plid);

    if ((dir = opendir(directory)) == NULL)
        mkdir(directory, 0777);
    else
        closedir(dir);

    char command[BUFFERSIZE];

    sprintf(command, "cp %s %s", filename, newfilename);

    system(command);

    if (code == 'W')
        create_score_file(plid, time, filename);
    else{
        sprintf(command, "rm %s", filename);
        system(command);
        free(time);
    }

}


void update_file(char *plid, char *add){
    ofstream outfile;
    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", plid);

    outfile.open(filename, ios_base::app); // update instead of overwrite
    outfile << add; 
    outfile.close();
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
    int val = countword;
    string tmp, w, h;
    int e=0;

    ifstream wordfile(wfile);   
    if (!wordfile) {            
        cout << "No word file was found1" << endl;
        exit(1);
    }
    while (e<=val){            
        getline(wordfile, tmp);
        e++;
    }
    wordfile.close();
    w = tmp.substr(0, tmp.find_first_of(" "));
    h = tmp.substr(tmp.find_first_of(" ")+1, tmp.length()-1);
    aux_word = create_string(&w[0]);
    aux_hint = create_string(&h[0]);
    if (countword==lines-1)
        countword=0;
    else
        countword++;
}

int letter_in_word(char* word, char letter, char* pos, int word_len){
    int hits=0;
    char add[3];
    for (int i=0; i<word_len; i++){
        if (toupper(word[i]) == toupper(letter)){
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

void writeTCP(int fd, char buffer[], ssize_t buffer_len){
    ssize_t nleft, nwritten;
    char* ptr = &buffer[0];
    nleft = buffer_len;
    while(nleft>0){
        nwritten=write(fd,ptr,nleft);
        if (nwritten==0)
            break;
        if(nwritten<0)/*error*/{
            exit(1);
        }
        nleft-=nwritten;
        ptr+=nwritten;
    }
}

void TCP_operations(int fd){
    int n, total;

	freeaddrinfo(resClientTCP);
	close(fdClientTCP);

    fdClientTCP=fd;
    
    char* f;
    char* ptr;

    while (1){
        memset(receiving, 0, BUFFERSIZE);
        ptr=&receiving[0];
        total=0;

        while ((n=read(fd, ptr, BUFFERSIZE-total))>=0){
            ptr += n;
            total += n;
            if (*(ptr-1) == '\n')
                break;
        }
        if (n == -1)
            exit(1);
        if (n == 0)
            break;
        f = strtok(receiving, " \n");
        if (strcmp(f, "GSB")==0)
            scoreboard();
        else if (strcmp(f, "GHL")==0){
            hint();
        }
        else if (strcmp(f, "STA")==0)
            state();
        else {
            memset(buffer, 0, BUFFERSIZE);
            strcpy(buffer, "ERR\n");
            writeTCP(fd, buffer, 4);
        }
    close(fd);
    }

}

bool validAlpha(char *string, size_t n){
    size_t a = strspn(string, "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz");
    if (a!=n){
        return false;
    }
    return true;
}

bool validPLID(char *string){
    size_t a = strspn(string, "0123456789");
    if (a==strlen(string))
        return true;
    return false;
}

bool duplicateplay(char* plid, char *f){
    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", plid);
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found2" << endl;
        exit(1);
    }
    string tmp;
    for (size_t i=0; i<strlen(f); i++) 
        f[0]=toupper(f[0]);
    while (getline(file, tmp)){
        transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
        if  (strcmp(tmp.c_str(), f)==0){
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}
bool islastplay(char* plid,char* move){
    char filename[BUFFERSIZE];
    sprintf(filename, "./GAMES/GAME_%s.txt", plid);
    
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found3" << endl;
        exit(1);
    }
    file.seekg(-1, ios_base::end);
    if(file.peek() == '\n')
    {
      file.seekg(-1, ios_base::cur);
      for(int i= file.tellg();i > 0; i--)
      {
        if(file.peek() == '\n')
        {
          file.get();
          break;
        }
        file.seekg(i, ios_base::beg);
      }
    }
    string tmp;
    getline(file, tmp);
    for (size_t i=0; i<strlen(move); i++) 
        move[0]=toupper(move[0]);
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    if (strcmp(tmp.c_str(), move)==0){
        file.close();
        return true;
    }
    file.close();
    return false;
}

bool compare_word(char* guess, char* word){
    for (size_t i=0; i<strlen(guess); i++)
        if (toupper(word[i])!=toupper(guess[i])){
            return false;
        }
    return true;
}

char* get_hint(char* filename){
    char* hint = (char*)calloc(BUFFERSIZE, sizeof(char));
    if (hint==NULL){
        exit(1);
    }
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found4" << endl;
        exit(1);
    }
    string tmp;
    getline(file, tmp);
    sscanf(tmp.c_str(), "%*s %s", hint);
    file.close();
    return hint;
}

char* get_word(char* filename){
    char* word = (char*)calloc(BUFFERSIZE, sizeof(char));
    if (word==NULL)
        exit(1);
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found5" << endl;
        exit(1);
    }
    string tmp;
    getline(file, tmp);
    sscanf(tmp.c_str(), "%s", word);
    file.close();
    return word;
}

bool has_active_game(char* plid){
    char buf[BUFFERSIZE];
    sprintf(buf, "./GAMES/GAME_%s.txt", plid);
    ifstream file(buf);
    if (!file)           
        return false;
    file.close();
    return true;
}

int get_thits(char* filename, char* word){
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found6" << endl;
        exit(1);
    }
    string tmp;
    int hit=0;
    getline(file, tmp);
    while (getline(file, tmp)){
        if (tmp.c_str()[0] == 'T' ){
            for (size_t i=0; i<strlen(word); i++){
                if (toupper(word[i]) == toupper(tmp.c_str()[2]))
                    hit++;        
            }
        }
    }
    file.close();
    return hit;
}

int get_attempt(char* filename){
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found" << endl;
        exit(1);
    }
    string tmp;
    int count=0;
    while (getline(file, tmp)){
        count++;
    }
    file.close();
    return count-1;
}

int get_errors(char* filename, char* word){
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found" << endl;
        exit(1);
    }
    int errors=0;
    int hit;
    string tmp;
    char w[BUFFERSIZE];
    getline(file, tmp);
    while (getline(file, tmp)){
        if (tmp.c_str()[0] == 'T' ){
            hit=0;
            for (size_t i=0; i<strlen(word); i++){
                if (toupper(word[i]) == toupper(tmp.c_str()[2]))
                    hit++;        
            }
            if(hit<1)
                errors++;
        }
        else{
            bzero(w, BUFFERSIZE);
            sscanf(tmp.c_str(), "%*c %s", w);
            if (strcmp(w, word)!=0)
                errors++;
        }
    }
    file.close();
    return errors;
}

int FindLastGame(char *plid, char *filename)
{
    struct dirent **filelist;
    int n_entries;
    char dirname [20];
    sprintf(dirname , "GAMES/%s/", plid) ;
    n_entries = scandir(dirname, &filelist, 0, alphasort);
    int found=0;

    if(n_entries <= 0) 
        return 0;
    else{
        while(n_entries--){
            if(filelist[n_entries] -> d_name[0]!= '.'){
                sprintf(filename, "GAMES/%s/%s", plid, filelist[n_entries] -> d_name);
                found=1;
            }
            free(filelist[n_entries]);
            if(found==1) 
                break;
        }
        free(filelist);
    }
    return found;
}