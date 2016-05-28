
#include "libs/serialize.h"
#include <stdio.h>

int main (void) {
    int a = 40;
    u_int32_t  b = 122;
    int c = 7777;
    u_int32_t  d = 8888;

    void * buff = NULL;

    size_t buffsize = 0;
    size_t lastIndex = 0;

    serialize_data(&a, sizeof(a), &buff, &lastIndex);
    printf("a: %d\n", *((int*)buff));

    serialize_data(&b, sizeof(b), &buff, &lastIndex);
    printf("b: %d\n", *((u_int32_t*) (buff+lastIndex-sizeof(b))));

    serialize_data(&c, sizeof(c), &buff, &lastIndex);
    printf("c: %d\n", *((int*)(buff+lastIndex-sizeof(c))));

    serialize_data(&d, sizeof(d), &buff, &lastIndex);
    printf("d: %d\n", *((u_int32_t*) (buff+lastIndex-sizeof(d))));

    // printf("Todo el contenido : %s\n", (char *) buff);

    a = 5; b = 5; c = 5; d = 5; lastIndex = 0;

    deserialize_data(&a, sizeof(a), buff, &lastIndex);
    printf("a: %d\n", a);
    deserialize_data(&b, sizeof(b), buff, &lastIndex);
    printf("b: %d\n", b);
    deserialize_data(&c, sizeof(c), buff, &lastIndex);
    printf("c: %d\n", c);
    deserialize_data(&d, sizeof(d), buff, &lastIndex);
    printf("d: %d\n", d);
    return 0;
}
