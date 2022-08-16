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

// ##############################
// ### Functions Over Strings ###
// ##############################

void random_string(char* string, int stringLen){
	char r;
	for(int i=0; i<stringLen; i++){
		r=rand()%95;
		*(string+i)=r+32;
	}
} // Puts a random string of printable characters in string. size stringLen

uc* random_bits(int k){
	uc* random;
	random=(uc*)malloc(k*sizeof(char));
	for(int i=0; i<k; i++){
		//*(random+i)=rand()%2+48;
		*(random+i)=rand()%2;
	}
	return random;
} // Creates an array of random bits (char types)

uc** createCharTabC(int nb_string, int size_string){
	uc** substring;
	substring=calloc(nb_string,sizeof(uc*));
	for(int i=0; i<nb_string; i++){
		*(substring+i)=calloc((size_string+1),sizeof(uc));
		random_string((char*)*(substring+i),size_string);
	}
	return substring;
} // Outputs an array of size nb_string. each block containing a random string 
  //of printable characters of size "size_string"

uc** createCharTabCWithTag(int nb_string, int size_string, int num_session){
	uc** substring;
	substring=(uc**)calloc(nb_string,sizeof(uc*)); 
	uc* temp=calloc(size_string,sizeof(uc));
	uc* tag_n;
	uc* tag_k;
	for(int i=0; i<nb_string; i++){
		tag_n=itouc2b(i);
		tag_k = itouc2b(num_session);
		*(substring+i)=(uc*)calloc((size_string+5),sizeof(uc));
		random_string((char*)temp,size_string);
		substring[i][0]=tag_k[0];
		substring[i][1]=tag_k[1];
		substring[i][2]=tag_n[0];
		substring[i][3]=tag_n[1];
		for(int j=0; j<size_string;j++){
			substring[i][j+4]=temp[j];
		}
		substring[i][size_string+4]='\0';
	}
	free(tag_n);
	free(tag_k);
	free(temp);
	return substring;
} // Outputs an array of size nb_string. each block containing a seq number called
  // tag written on 2 bytes and a random string of printable characters of size 
  // "size_string"

uc* extract_string(int from, int to, uc* string, int lenght){
	int i=0, j=0;
	//int lenght =strlen(string);
	if(from > to){
		printf("/!\\ Error on extract_string : 'from' is higher than 'to'\n");
		return NULL;
	}

	if(from > lenght || from < 0){
		printf("/!\\ Error on extract_string : invalid index 'from'\n");
		return NULL;
	}
	if(to > lenght){
		printf("/!\\ Error on extract_string : invalid index 'to'\n");
		return NULL;
	}

	uc* subString;
	subString=(uc*)calloc((to-from)+1,sizeof(uc));

	for(i=from, j=0; i<= to; i++, j++){
		subString[j] = string[i];
	}
	subString[to-from]='\0';
	return subString;
} // Extract a substring between indexes from and to (included)
  // example : extract_string(2,5,"bonjour\0") returns "njou\0"
  // Does not work properly on uc containing meaningful "0x00" values

void mess_and_tag(int lenght, uc* tag_k, uc* tag_n, uc*** mess, uc* buff){
	if(tag_k != NULL){
		tag_k[0]=buff[0];
		tag_k[1]=buff[1];
	}
	if(tag_n != NULL){
		tag_n[0]=buff[2];
		tag_n[1]=buff[3];
	}
	int itag_k = uc2btoi(tag_k);
	int itag_n = uc2btoi(tag_n);
	for(int i=4; i<lenght; i++){
		mess[itag_k][itag_n][i-4]=buff[i];
	}
}// extract the value of tag and mess from the buffer = [tag_1,tag_2,(mess)]

void bit_and_tag(uc* tag_k, uc* tag_n, uc* bit, uc* buff){
	tag_k[0]=buff[0];
	tag_k[1]=buff[1];
	tag_n[0]=buff[2];
	tag_n[1]=buff[3];
	*bit=buff[4];
}// extract the value of tag and bit from the buffer = [tag_1, tag_2, bit]

void add_mess(uc** messages, int index, uc* mess, int size_mess){ //REMPLACE add_len_to_view
	for(int i=0; i<size_mess; i++){
		messages[index][i]=mess[i];
	}
}// Add mess of size ''size_mess'' at messages[index]

