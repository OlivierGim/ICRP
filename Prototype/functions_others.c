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

// ###################################
// ### Fonctions Aide Utilisateurs ###
// ###################################

void usage_sender(void){
	printf("#############\n");
	printf("### USAGE ###\n");
	printf("#############\n");
	printf("\n");
	printf("./Sender.out <options> <ip:port>\n");
	printf("This program sends UDP packets to the specified IP on the specified port number.\n\
A given number of packets is sent upon receiving a response from the specified IP.\n\
Each RTT is computed and stored in a table that can be displayed or saved in txt format.\n");
	printf("Options: [-p packet size]\n\
         [-n number of packets for one sample]\n\
         [-k number of sample to generate]\n\
         [-R random lenght interval semi-size]\n\
         [-m traffic mode selection: 0 -> normal, 1 -> mixed , 2 -> hijacked]\n\
         [-l indicates the location of the Sender to appear in .txt summary]\n\
         [-s indicates the time to wait between each individual sendings (in ms)]\n\
         [-I ip:port for hijacker]\n\
         [-S Save the gathered samples on .txt files]\n");
} // Display on -h

void usage_receiver(void){
	printf("#############\n");
	printf("### USAGE ###\n");
	printf("#############\n");
	printf("\n");
	printf("./Receiver.out <port> <number of round> <geographical location>\n");
	printf("This program receives UDP packets on the specified port number and answers ''Confirm'' so the Sender can compute RTTs.\n");
} // Display on -h

// ###################################
// ### Display For Debug Functions ###
// ###################################

void tab_display_ul(unsigned long *tab, int tablen){
	printf("[");
	for(int i=0; i<tablen; i++){
		printf("%lu",*(tab+i));
		if(i!=tablen-1){
			printf(",");
		}
	}
	printf("]\n");
}// Affiche un tableau d'unsigned long

void tab_display_uc(unsigned char* tab, int len){
	printf("[");
	for(int i=0; i<len; i++){
		printf("%02x",*(tab+i));
		if(i!=len-1){
			printf(",");
		}
	}
	printf("]\n");
}