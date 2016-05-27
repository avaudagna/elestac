//
// Created by alan on 26/05/16.
//

#include "libs/serialize.h"
#include <stdio.h>
#include <string.h>

int main (void) {
    int a = 40;
    u_int32_t  b = 122;
    int c = 7777;
    u_int32_t  d = 8888;

    char * buff = NULL;//malloc(sizeof(int));
    char * cursor = buff;
    cursor = serialize_data(&a, sizeof(a), &buff);
    printf("a: %d\n", atoi(buff));

    return 0;
}