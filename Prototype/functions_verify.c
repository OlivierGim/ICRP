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
#include <openssl/sha.h>
// #include <openssl/evp.h>
// #include <openssl/ssl.h>
// #include <openssl/rsa.h>
// #include <openssl/x509.h>
#include "functions.h"
#define MAXLINE 4096

typedef unsigned char uc;

// ##################################
// ### Cryptographic Computations ###
// ##################################

char string_comp(char *s1, char *s2){// C'est là parce que ça ne sert que dans la verif de signature
	char comp=1;
	int i=0;
	if(strlen(s1)!=strlen(s2)){

		return 0;
	}
	while(comp==1 && *(s1+i)!=0){
		if(*(s1+i)!=*(s2+i)){
			comp=0;
			//printf("Le %dème caractère est différent : %d,%d,\n",i,*(s1+i),*(s2+i));
			return 0;
		}
		i++;
	}
	return 1;
} // À supprimer on utilisera memcmp(void* s1, void* s2, size_t n);
  // ça compare les n bytes (interprété en unsigned char) suivant 
  // les addresses pointées par s1 et s2

// #######################################
// ### Threshold and Decision Function ###
// #######################################

int get_file_size(FILE* f){
	int size=0;
	fseek(f,0L, SEEK_END);
	size=ftell(f);
	fseek(f,0L,SEEK_SET);
	return size;
} //return the size of a file

int get_line(char* str, int size, FILE* f){
	int c;
	int counter=0;
	for(int i=0; i<size; i++){
		//printf("%c",fgetc(f));
		c=fgetc(f);
		if(c==',' || c==';') counter++;
		str[i]=(char)c;
	}
	return counter;
}// stores in str the first line of the file f (ref_files have only one line)
 // returns the number of numerical elements in the file (as ref files are in csv format)

void tab_from_line(int* tab, int tabsize, char* str){
	char* token;
	char delimiters[4]=", ;";
	token = strtok(str,delimiters);
	tab[0]=atoi(token);
	for(int i=1; i<tabsize; i++){
		token = strtok(NULL,delimiters);
		tab[i]=atoi(token);	
	}
}// stores the integer array from the litteral string [nb_1, ..., nb_n ; nb_1, ..., nb_n ; etc ...] in str

char Decide(unsigned long* rtts, int tablen, int thresh, float p_accept){
	int count=0;
	float prop;
	for(int i=0; i<tablen; i++){
		if(rtts[i]>thresh){
			count++;
		}
	}
	prop=(float)count/(float)tablen;
	printf("   Proportion au dessus du seuil: %f\n",prop);
	return prop>=p_accept;
}// Given a threshol, a proportion of acceptation, and a list of times, return 0 or 1 whether
 // the proportion p of elements of rtts being above thresh, is greater than p_accept

int compare(const void* a, const void* b){
	int int_a = * ((int*) a);
	int int_b = * ((int*) b);

	if(int_a==int_b) return 0;
	else if(int_a < int_b) return -1;
	else return 1;
}//Compares two int value (so we can use the built-in qsort function)

// #################
// ### Timestamp ###
// #################

unsigned long timestamp(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long time_in_micro= 1000000 * tv.tv_sec + tv.tv_usec;
	return time_in_micro;
}// Outputs Timestamp in microseconds