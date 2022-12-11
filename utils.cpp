#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

using namespace std;



bool validPLID(char *string)
{
    int a = strspn(string, "0123456789");
    if (a==strlen(string))
        return true;
    return false;
}

/*funÇão que retorna o número de digitos */
int count_digit(int n){
    int count;
    while(n != 0){
        n=n/10;
        count++;
    }
    return count;
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
        /*gera erro*/
    }

    return max_errors;
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