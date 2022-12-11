#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool validPLID(char *string);
int count_digit(int n);
int get_max_errors(char *word);
char* create_string(char* p);
bool validAlpha(char *string, size_t n);

#endif /*UTILS_H*/