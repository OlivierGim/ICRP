#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
// #include <openssl/evp.h>
// #include <openssl/ssl.h>
// #include <openssl/rsa.h>
// #include <openssl/x509.h>
#include "functions.h"
#define MAXLINE 4096

typedef unsigned char uc;

// ################################
// ### Functions For Conversion ###
// ################################

uc* itouc2b(int num){
	if(num>=65536){
		printf("/!\\ in function: sizeuc, too large int\n");
		return 0;
	}
	uc* hexa;
	hexa = calloc(2,sizeof(uc));
	hexa[0] = (num & 0xff00) >>  8;
	hexa[1] = (num & 0x00ff)      ;
	return hexa;
}//Converts int lesser than 65635 to its corresponding 2 bytes array

int uc2btoi(uc* hexa){
	int num;
	num=(int)hexa[0]*256+hexa[1];
	return num;
}//Converts an array of 2 bytes to its corresponding int value

// #######################################
// ### Infos Extraction from arguments ###
// #######################################

char* atoIP(char* info){
	char* IP;
	IP=(char*)calloc(16,sizeof(char));
	int i=0;
	while(*(info+i)!=':'){
		*(IP+i)=*(info+i);
		i++;
	}
	return IP;
}// output char* IP from input argument "IP:port"

int atoPort(char* info){
	char* portStr;
	portStr=(char*)calloc(6,sizeof(char));
	int port;
	int i,j;
	i=0;
	j=0;
	while(*(info+i)!=':'){
		i++;
	}
	i++;
	while(*(info+i)!='\0'){
		*(portStr+j)=*(info+i);
		i++;
		j++;
	}
	port=atoi(portStr);
	free(portStr);
	return port;

}// output int Port from input argument "IP:port"