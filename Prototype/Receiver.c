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
#include <pthread.h>
#include "functions.h"
#define MAXLINE 1024

typedef unsigned char uc;

// struct shared{
// 	uc* view; // [mess[0]|bit[0],...,mess[n-1]|bit[n-1]]
// 	int n;
// 	int k;
// 	int sockfd
// 	socklen_t respLen;
// 	struct sockaddr_in Receiver,Sender;
// 	pthread_mutex_t mut;
// };

// static void* task_resp{
// 	if(p!=NULL){
// 		int size; //size of received messages
// 	}
// };

int main(int argc, char* argv[]){
	// ### VARIABLES ###
	// Getopt
	int option;

	// Sockets & Network
	int sockfd;
	socklen_t len=0;
	int size;
	struct sockaddr_in Sender, Receiver;
	int port=12345; // Default value

	// Buffer, bits, messages, lenght and tag
	unsigned char* buffer;
	buffer = calloc(MAXLINE,sizeof(unsigned char));
	unsigned char* resp;
	resp = calloc(3,sizeof(unsigned char));
	char* bits;
	unsigned char* mess;
	mess = calloc(MAXLINE,sizeof(unsigned char));
	unsigned char** messages;
	unsigned char* tag_2b;
	int tag_i;
	tag_2b = calloc(2,sizeof(unsigned char));
	int lenght=-1;

	//Cryptographic data
	uc* hash = calloc(32,sizeof(uc));
	uc* sign;
	int sign_size;

	// Parameters
	int n,p,k;
	n=256;
	p=512;   // Default values
	k=1;

	// Seed for rand gen
	srand(time(NULL));
	

	// ### GETOPT ###
	while((option=getopt(argc,argv,"hn:p:k:P:"))!=-1){// On indique les lettres des opts et on rajoute
												 // ":" après la lettre si cette opt necessite un
												 // argument
		switch(option){
			case 'h':
				printf("Help Display\n");
				return 0;
			case 'n':
				n = atoi(optarg); //size of one sample
				break;
			case 'p':
				p = atoi(optarg); //size of one packet
				break;
			case 'k':
				k = atoi(optarg); //how many sample to collect before stop
				break;
			case 'P':
				port=atoi(optarg); //port number
				break;
		}
	}
	//Memory allocation
	messages = calloc(n,sizeof(unsigned char*));
	for(int i=0; i<n; i++){
		messages[i]=calloc(p,sizeof(unsigned char));
	}
	bits = random_bits(n);

	//Socket file descriptor creation
	if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0){ //socket renvoie -1 en cas d'echec
		///printf('TEST : %d\n',sockfd)
		perror("Echec de la creation de la socket");
		exit(EXIT_FAILURE);
	}
	memset(&Receiver, 0, sizeof(Receiver)); //initialise à 0 la zone mémoire de la structure Receiver
	memset(&Sender, 0, sizeof(Sender)); //initialise à 0 la zone mémoire de la structure Sender
	
	//Server info
	Receiver.sin_family = AF_INET; //IPv4
	Receiver.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY pour lier socket à toutes interfaces 
										   //disponibles
	Receiver.sin_port = htons(port); //htons() est unsigned short int [0;2^16[, c'est juste une conversion
	
	//Link socket with serv addr
	if(bind(sockfd,(const struct sockaddr *)&Receiver,sizeof(Receiver)) < 0){
		perror("Echec de creation du lien");
		exit(EXIT_FAILURE);
	}

	// Receive and respond
	for(int nb_samp=0; nb_samp<k; nb_samp++){
		len=sizeof(Sender);
		printf("Waiting . . .\n");
		for(int i=0; i<n; i++){
			size=recvfrom(sockfd,(unsigned char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&Sender,&len);
				buffer[size]='\0';
				//lenght=size-2;
				mess_and_tag(lenght,tag_2b,mess,buffer);
				tag_i=uc2btoi(tag_2b);
				// add_uc_to_tab(view,tag_i,mess,lenght);
				add_mess(messages,tag_i,mess,p);
				resp[0]=tag_2b[0];
				resp[1]=tag_2b[1];
				resp[2]=bits[tag_i];
			sendto(sockfd, (const unsigned char*)resp, 3, MSG_CONFIRM, (const struct sockaddr *)&Sender,len);
			if(n==-1){
				printf("/!\\ Socket Timed out\n");
				return -1;
			}
			//printf("\n");
			// ### À la fin de cette boucle on a le tableau des messages reçu dans l'ordre
			// ### défini par les tags et le tableaux des bits. On doit écrire ça dans un
			// ### fichier et hash & sign
		}
		printf("\n");

		printf("sample of size %d received, processing hash and sign...\n",n);
		Hash(messages,bits,n,p,hash);
		//hash=HashViewReceiver(view,bits,n);
		//sign=Sign(hash,&sign_size);
		// printf("La Signature:\n");
		// tab_display_uc(sign,sign_size);
		free(hash);
		printf("... DONE !\n");
		for(int i=0; i<n; i++){
			memset(messages[i],0,p);
		}
		sendto(sockfd,(const unsigned char *)sign,sign_size,MSG_CONFIRM,(const struct sockaddr *)&Sender,sizeof(Sender));
		printf("Signature sent\n");
		free(sign);
		printf("\n");
	}
	return 0;
}

/*

*/
