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
void create_ongoing_gamefile(char *plid, char *word, char *hint, int thits, int max_errors);
void update_file(char *plid, char *add, char *word, char* hint, int attempt, int thits, int errors, int max_errors);
void start();
void play();
void guess();
void state();
void hint();
void quit_exit();
void scoreboard();
bool validPLID(char *string);
bool validAlpha(char *string, size_t n);
int get_max_errors(char* word);
void choose_word();
int letter_in_word(char* word, char* letter, char* pos, int word_len);
void writeTCP(int fd, char buffer[], ssize_t buffer_len);
bool duplicateplay(char* plid, char *f);
bool compare_word(char* guess, char* word);
char* get_hint(char* plid);
bool has_active_game(char* plid);
void get_variables_from_file(char* plid, char *word, char* hint, int *attempt, int *thits, int *errors, int *max_errors);
bool islastplay(char* plid,char* move);
int findSize(char file_name[]);

int main(int argc, char *argv[]){
    readInput(argc, argv);
    initGSUDP();
    initGSTCP();

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
    int thits;
    int max_errors;
    int attempt;
    int errors;
    
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
        printf("main: %s\n", aux_hint);
        printf("W: %s\n",aux_word);
        thits = strlen(aux_word); 
        max_errors = get_max_errors(aux_word);
        if (max_errors==-1){
            cout << "Invalid word size" << endl;
            exit(1);
        }
        strcpy(status,"OK");
        num = sprintf(sending, "RSG %s %ld %d\n", status, strlen(aux_word), max_errors);
        create_ongoing_gamefile(plid, aux_word, aux_hint, thits, max_errors);
        free(aux_word);
        free(aux_hint);
    }
    else{
        aux_word=(char*) calloc(BUFFERSIZE, sizeof(char));          
        if (aux_word==NULL){
            exit(1);
        }
        aux_hint=(char*) calloc(BUFFERSIZE, sizeof(char));          
        if (aux_hint==NULL){
            exit(1);
        }
        get_variables_from_file(plid, aux_word, aux_hint, &attempt, &thits, &errors, &max_errors);
        if (attempt>0){
            strcpy(status,"NOK");
            num = sprintf(sending, "RSG %s\n", status);
        }
        else{
            strcpy(status,"OK");
            num = sprintf(sending, "RSG %s %ld %d\n", status, strlen(aux_word), max_errors);
        }
        free(aux_word);
        free(aux_hint);
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
    char *word=(char*) calloc(BUFFERSIZE, sizeof(char));          
    if (word==NULL){
        exit(1);
    }
    char *hint=(char*) calloc(BUFFERSIZE, sizeof(char));         
    if (hint==NULL){
        exit(1);
    }

    id = strtok(NULL, " \n");

    get_variables_from_file(id, word, hint, &attempt, &thits, &errors, &max_errors);
    char* pos = (char*)calloc((strlen(word)*2), sizeof(char));
    if (pos == NULL){
        perror("Error: ");
        exit(1);
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
    sprintf(move, "T %c", toupper(letter[0]));
    if (val==attempt){
        if(islastplay(id, move)){
            strcpy(status,"OK");
            int hits = letter_in_word(word, letter, pos, strlen(word)); 
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
        int hits = letter_in_word(word, letter, pos, strlen(word)); 
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
        update_file(id, move, word, hint, attempt, thits, errors, max_errors);

    }
    
    printf("SENDING: %s", sending);
    
    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }
    free(pos);
    free(word);
    free(hint);
}

void guess(){           
    ssize_t n;
    int num;
    char* guess;
    char* id;
    char status[4];
    int val;
    int attempt;
    int thits;
    int errors;
    int max_errors;
    char move[BUFFERSIZE];
    char *word=(char*) calloc(BUFFERSIZE, sizeof(char));           
    if (word==NULL){
        exit(1);
    }
    char *hint=(char*) calloc(BUFFERSIZE, sizeof(char));            
    if (hint==NULL){
        exit(1);
    }

    id = strtok(NULL, " \n");
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

    get_variables_from_file(id, word, hint, &attempt, &thits, &errors, &max_errors);

    sprintf(move, "T %s", guess);
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
            strcpy(status, "WIN");              //acabar jogo e fazer ficheiro de score
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
        update_file(id, move, word, hint, attempt, thits, errors, max_errors);
    }
    
    num = sprintf(sending, "RWG %s %d\n", status, attempt);
    printf("SENDING: %s", sending);
    
    n = sendto(fdClientUDP, sending, num, 0, (struct sockaddr *) &addr, addrlen);
    if (n==-1){
        cout << "Unable to send from server to user" << endl;
        exit(1); 
    }

    free(word);
    free(hint);
}

void hint(){
    int num;
    char* id;
    long int fsize;
    size_t n;

    id = strtok(NULL, " \n");
    char* fname = get_hint(id);
    memset(sending, 0, BUFFERSIZE);
    FILE *file = fopen("test.jpg", "r");             //Alterar no fim
    if (file == NULL){
        num = sprintf(sending, "RHL NOK\n");
        writeTCP(fdClientTCP, sending, num);
        return;
    }
    fseek(file, 0L, SEEK_END);
    fsize = ftell(file);
        
    num = sprintf(sending, "RHL OK %s %ld ", fname, fsize);
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

    printf("entrou no quit\n");

    id = strtok(NULL, " \n");
    printf("%s\n", id);
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

    
    if (has_active_game(id))
        strcpy(status, "OK");
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



void scoreboard(){

}

void state(){

    /*int num;
    char* id;
    long int fsize;
    size_t n;

    id = strtok(NULL, " \n"); 

    if (!validPLID(id)){
        printf("Invalid PLID");
        exit(1);
    }

    if (has_active_game()


    char* fname = get_hint(id);
    memset(sending, 0, BUFFERSIZE);
    FILE *file = fopen("test.jpg", "r");             //Alterar no fim
    if (file == NULL){
        num = sprintf(sending, "RST NOK\n");
        writeTCP(fdClientTCP, sending, num);
        return;
    }
      
     fseek(file, 0, SEEK_END);
	fsize = ftell(file);
    printf("hint size: %ld\n", fsize); */
    /* fseek(file, 0L, SEEK_END);
    fsize = ftell(file);
        
    num = sprintf(sending, "RHL OK %s %ld ", fname, fsize);
    printf("sending: %s", sending);
    writeTCP(fdClientTCP, sending, num);
    fclose(file);

    printf("chegou ao loop\n");
    file = fopen("test.jpg", "rb"); 
    while (fsize>0){
        memset(sending, 0, BUFFERSIZE);
        n = fread(sending, 1, BUFFERSIZE, file);
        fsize-=n;
        writeTCP(fdClientTCP, sending, n);
        printf("sending: %s\n", sending);
        printf("bytes: %ld\n", n);
    }
    memset(&sending[0], '\n', 1);
    writeTCP(fdClientTCP, sending, 1);
    fclose(file); */

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
    int n;

    fdClientTCP = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if(fdClientTCP == -1)
        exit(1);
    memset(&hintsClientTCP,0,sizeof hintsClientTCP);            //talvez separar
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

void create_ongoing_gamefile(char *plid, char *word, char *hint, int thits, int max_errors){
    char filename[BUFFERSIZE];
    sprintf(filename, "GAME_%s.txt", plid);
    ofstream outfile (filename);
    char line2[]="0";
    char line3[7];
    sprintf(line3, "%d 0 %d", thits, max_errors);
    outfile << word << " " << hint << " " << line2 << " " << line3 << endl;
    outfile.close();
}

void update_file(char *plid, char *add, char *word, char* hint, int attempt, int thits, int errors, int max_errors){
    ofstream outfile;
    stringstream buff;
    string s;
    char filename[BUFFERSIZE];
    sprintf(filename, "GAME_%s.txt", plid);

    ifstream sfile (filename);
    buff << sfile.rdbuf();
    s=buff.str();
    s = s.substr(s.find_first_of("\n")+1, s.length()-1);
    s.append(add);
    sfile.close();
    
    FILE* file = fopen(filename, "w");
    sprintf(buffer, "%s %s %d %d %d %d\n", word, hint, attempt, thits, errors, max_errors);
    fputs (buffer, file);
    fclose(file);

    outfile.open(filename, ios_base::app); // update instead of overwrite
    outfile << s.c_str(); 
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
    h = tmp.substr(tmp.find_first_of(" ")+1, tmp.length()-1);
    aux_word = create_string(&w[0]);
    aux_hint = create_string(&h[0]);
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

void writeTCP(int fd, char buffer[], ssize_t buffer_len){
    ssize_t nleft, nwritten;
    char* ptr = &buffer[0];
    nleft = buffer_len;
    while(nleft>0){
        nwritten=write(fd,ptr,nleft);
        if (nwritten==0)
            break;
        if(nwritten<0)/*error*/{
            printf("FDS\n");
            exit(1);
        }
        nleft-=nwritten;
        ptr+=nwritten;
        printf("nwritten: %ld\n", nwritten);
    }
    printf("SENDING TCP: %s", buffer);
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
    sprintf(filename, "GAME_%s.txt", plid);
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found" << endl;
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
    sprintf(filename, "GAME_%s.txt", plid);
    
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found" << endl;
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
    printf("last: %s\n", tmp.c_str());
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

char* get_hint(char* plid){
    char filename[BUFFERSIZE];
    char* hint = (char*)calloc(BUFFERSIZE, sizeof(char));
    sprintf(filename, "GAME_%s.txt", plid);
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found" << endl;
        exit(1);
    }
    string tmp;
    getline(file, tmp);
    sscanf(tmp.c_str(), "%*s %s", hint);
    file.close();
    return hint;
}

bool has_active_game(char* plid){
    char buf[BUFFERSIZE];
    sprintf(buf, "GAME_%s.txt", plid);
    ifstream file(buf);
    if (!file)           
        return false;
    file.close();
    return true;
}

void get_variables_from_file(char* plid, char *word, char* hint, int *attempt, int *thits, int *errors, int *max_errors){
    char filename[BUFFERSIZE];
    sprintf(filename, "GAME_%s.txt", plid);
    ifstream file(filename);
    if (!file) {            
        cout << "No word file was found" << endl;
        exit(1);
    }
    string tmp;
    getline(file, tmp);
    sscanf(tmp.c_str(), "%s %s %d %d %d %d", word, hint, attempt, thits, errors, max_errors);
    file.close();
}

