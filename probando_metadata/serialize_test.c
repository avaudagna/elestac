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

    char * buff = malloc(1);
    buff = serialize_data(&a, sizeof(a), &buff);
    printf("a: %d\n", (int) *buff);

    buff = serialize_data(&b, sizeof(b), &buff);
    printf("b: %d\n", (u_int32_t) *(buff+sizeof(a)));


    buff = serialize_data(&c, sizeof(c), &buff);
    printf("c: %d\n", (int) *(buff+sizeof(b)+sizeof(a)));
/*
    buff = serialize_data(&d, sizeof(d), &buff);
    printf("d: %d\n", (u_int32_t) *(buff+sizeof(b)+sizeof(a)+sizeof(c)));

    printf("todo: %s\n", buff);*/

    return 0;
}