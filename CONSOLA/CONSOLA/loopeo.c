#include <stdio.h>
#include <string.h>

#define PACKET_SIZE 10

int main (void) {
	char src[40];
	char dest[PACKET_SIZE + 1];
	int packLen = (int) (sizeof(src)/sizeof(src[0]));
	while (packLen != 0){
		if(packLen>= PACKET_SIZE){
			strncpy (dest, PACKET_SIZE, src);
			send (dest, PACKET_SIZE+1);
			src = src + PACKET_SIZE;
			packLen = packLen - PACKET_SIZE;

		}
		else{
			strncpy (dest, packLen, src);
			send(dest);
			packLen = 0;
		}
	}
	printf("%s\n, sizeof %d\n", hola, sizeof(hola[1]) );
	strcpy(src, "Hoy vinimos a trabajar en el tp de oper");
	//while()
	memset(dest, '\0', sizeof(dest));
	strncpy(dest, src, 10);
	printf("%s\n", dest);
	return 0;
}
