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
void connectTCP();
void writeTCP(int fd, char buffer[], ssize_t buffer_len);
void readTCP(int fd, char *buffer, ssize_t len);
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
    connectTCP();

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
    

    memset(msg, 0, BUFFERSIZE);
    attempt++;
    num = sprintf(msg, "PLG %s %s %d\n", plid, letter, attempt);
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

    attempt = atoi(strtok(NULL, " \n"));

    if (strcmp(status, "ERR")==0){
        printf("ERROR WITH COMMAND\n");
        return;
    } 
    else if (strcmp(status, "DUP")==0){         //NÂO INCREMENTAR ATTEMPT
        printf("JOGADA DUPLICADA\n");
        return; 
    }
    else if (strcmp(status, "WIN")==0){
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
    
    printf("Yes,\"%s\" is part of the word: %s\n", letter, l);
}

void guess(){
    ssize_t n;
    int num;
    char* f;
    char* status;
    char* word = strtok(NULL, " \n");
    
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

    f = strtok(receiving, " \n");

    if (strcmp(f, "RWG")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    status = strtok(NULL, " \n");

    attempt = atoi(strtok(NULL, " \n"));

    if (strcmp(status, "ERR")==0){
        printf("ERROR WITH COMMAND\n");
        return;
    } 
    else if (strcmp(status, "WIN")==0){
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

}

void hint(){

    ifstream image;
    ssize_t n;
    int num;
    char* ptr;
    char* f;
    char* status;
    char* fname;
    char* sfsize;
    char* fdata;
    int total;
    memset(msg,0,BUFFERSIZE);
    num = sprintf(msg, "GHL %s\n", plid);
    
    
    printf("SENDING: %s", msg);
    writeTCP(fdServerTCP, msg, num);

    memset(receiving, 0, BUFFERSIZE);
    total=0;
    ptr = &receiving[0];
    while ((n=read(fdServerTCP, ptr, BUFFERSIZE-total))!=0){
        if (n == -1){
            exit(1);
        }
        ptr += n;
        total += n;
        if (*(ptr-1) == '\n'){
            break;
        }
    }
    printf("ENVIADO\n");
    close(fdServerTCP);
    printf("RECEIVING: %s\n", receiving);
    f = strtok(receiving, " \n");
    if (strcmp(f, "RHL")!=0){
        cout << "Wrong return message from server to user" << endl;
        exit(1); 
    }

    status = strtok(NULL, " \n");
    if (strcmp(status, "ERR")==0){
        printf("ERROR WITH COMMAND\n");
        return;
    } 
    else if (strcmp(status, "NOK")==0){
        cout << "Error getting hint" << endl;
        return; 
    }
    else if (strcmp(status, "OK")!=0){
        cout << "Unexpected response" << endl;
        exit(1); 
    }
    
    fname = strtok(NULL, " \n");
    printf("name: %s\n", fname);

    sfsize = strtok(NULL, " \n");
    printf("sizec: %s\n", sfsize);
    printf("sizei: %d\n", atoi(sfsize));

    fdata = strtok(NULL, " \n");
    printf("data: %s\n", fdata);
    
    ofstream fp(fname);
    fp << fdata;
    /* if (fwrite(fdata,sizeof(Byte),atoi(sfsize), fp)==0)
        printf("Failed to write\n"); */

    total=atoi(sfsize)-sizeof(*fdata);
    memset(receiving, 0, BUFFERSIZE);
    while (total > 0){
        ptr = &receiving[0];
        n=read(fdServerTCP, ptr, BUFFERSIZE);
        if (n == -1){
            exit(1);
        }
        total -= n;
        ptr += n;
        if (*(ptr-1) == '\n'){
            break;
        }
        fp << receiving;
    }

    fp.close();

    image.open(fname); //abre a foto da hint

    /*recebe hint ou h do input
    
    estabelece uma conexão TCP
    
    envia "GHL (plid)\n"
    
    recebe "RHL (status) (Fname Fsize Fdata)\n"
    */
}

void state(){
    /*recebe state ou st do input
    
    estabelece uma conexão TCP

    envia 
    */
}

void quit(){

    ssize_t n;
    int num;
    char f[3];
    char status[3];
    char* splid = strtok(NULL, " \n");

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "QUT %s\n", splid);
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
    
    fecha as conexões TCP*/
}

void exit(){

    ssize_t n;
    int num;
    char f[3];
    char status[3];
    char* splid = strtok(NULL, " \n");

    memset(msg, 0, BUFFERSIZE);
    num = sprintf(msg, "QUT %s\n", splid);
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
    
    fecha as conexões TCP*/
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
    if (fdServerTCP==-1) exit(1); //error     
}

void connectTCP(){
    ssize_t n;

    memset(&hintsServerTCP,0,sizeof hintsServerTCP);
    hintsServerTCP.ai_family=AF_INET;
    hintsServerTCP.ai_socktype=SOCK_DGRAM;

    errcode= getaddrinfo(GSip, GSport, &hintsServerTCP, &resServerTCP);
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

void readTCP(int fd, char *buffer, ssize_t len){
    char* ptr;
    ssize_t nleft, nread;
    nleft=len; 
    ptr=buffer;
    while(nleft>0){
        nread=read(fd,ptr,nleft);
        if(nread==-1)/*error*/
            exit(1);
        else if(nread==0)
            break;//closed by peer
        nleft-=nread;
        ptr+=nread;}
        nread=len-nleft;
}