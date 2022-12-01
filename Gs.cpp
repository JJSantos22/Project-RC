#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define GN 60
#define PORT 58000

//Global Variables
int verbose;
char *GSport;

void readInput(int argc, char *argv[]);
void initGSUDP();
void initGSTCP();
void initDB();

int main(int argc, char *argv[])
{
    readInput(argc, argv);
    initGSUDP();
    initGSTCP();
    initDB();

}

void readInput(int argc, char *argv[]){
    int e=1;
    verbose = 0;

    while (e < argc){
        if (strcmp(argv[e], "-p") == 0){
            GSport = (char *) malloc(6 * sizeof(char));
            strcpy(GSport, argv[e + 1]); //atenção!! a rever
        }
    }



    
}

void initGSUDP(){
    
}

void initGSTCP(){}

void initDB(){
    DIR *dir;
    if ((dir = opendir("USERS")) == NULL)
        mkdir("USERS", 0777);
    else
        closedir(dir);
}
