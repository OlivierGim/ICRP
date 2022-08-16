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
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "functions.h"
#define MAXLINE 2048

typedef unsigned char uc;

struct shared{
	int n; // number of rounds
	int p; // packet size
	int k; // number of sessions

	EVP_MD_CTX** ctx; //context for storage of hash values
	unsigned char*** messages;
	unsigned char** bits;

	int sockfd; //sock file descriptor
	struct sockaddr_in Receiver;
	struct sockaddr_in Sender;
	int sockfd_sig;
	struct sockaddr_in Sender_sig;

	pthread_mutex_t mut_mess_rcvd;
	pthread_cond_t cond_mess_rcvd;
	int mess_rcvd;
}; // All the variables common to the different Threads

static void* recving(void* p){
	if(p != NULL){
		struct shared* sh = p;

		//local
		int bufsize;
		uc* buffer = calloc(MAXLINE,sizeof(uc));
		//int fnt=0; // first index between 0 and nk-1 non treated
		socklen_t addrlen = sizeof(sh->Sender);
		uc* uctag_k = calloc(2,sizeof(uc));
		uc* uctag_n = calloc(2,sizeof(uc));
		int itag_k;
		int itag_n;
		uc* resp = calloc(5,sizeof(uc));
		//int tmp=-1;

		printf("Waiting . . .\n");
		for(int j=0; j< sh->k; j++){
			for(int i=0; i<sh->n; i++){
				bufsize=recvfrom(sh->sockfd,(uc *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&(sh->Sender),&addrlen);
				buffer[bufsize]=0;
				mess_and_tag(bufsize,uctag_k,uctag_n,sh->messages,buffer);
				itag_k = uc2btoi(uctag_k);
				itag_n = uc2btoi(uctag_n);
				resp[0] = uctag_k[0];
				resp[1] = uctag_k[1];
				resp[2] = uctag_n[0];
				resp[3] = uctag_n[1];
				resp[4] = sh->bits[itag_k][itag_n];
				sendto(sh->sockfd, (const uc*)resp, 5, MSG_CONFIRM, (const struct sockaddr *)&(sh->Sender),sizeof(sh->Sender));
				// if(fnt == sh->n*itag_k + itag_n){
				// 	fnt++;
				// 	while(sh->messages[fnt/sh->n][fnt%sh->n][0] != 0){
				// 		fnt++;
				// 	}
				// }
				// if(tmp != (sh->mess_rcvd = (fnt/sh->n) - 1)){
				// 	printf("sample %d recv !!\n",fnt/sh->n - 1);
				// 	pthread_cond_signal(&sh->cond_mess_rcvd);
				// 	tmp = sh->mess_rcvd;
				// }
			}
			sh->mess_rcvd++;
			//printf("sample %d recv !!\n",sh->mess_rcvd);
			pthread_cond_signal(&sh->cond_mess_rcvd);
		}
		free(buffer);
		free(uctag_k);
		free(uctag_n);
		free(resp);
	}
	return 0;
}

static void* signing(void *p){
	if(p != NULL){
		struct shared* sh = p;

		//local
		size_t * slen = calloc(1,sizeof(size_t));
		*slen = (size_t) 256; //signature len
		uc* sig = NULL;
		int ret = 0;
		if(!(sig = OPENSSL_malloc(sizeof(uc) * 256))){
				printf("/!\\ Error while allocating for signature\n");
			}
		
		for(int j=0; j<sh->k; j++){
			if(sh->mess_rcvd<j){
				pthread_mutex_lock(&sh->mut_mess_rcvd);
				pthread_cond_wait(&sh->cond_mess_rcvd, &sh->mut_mess_rcvd);
				pthread_mutex_unlock(&sh->mut_mess_rcvd);
			}
			for(int i=0; i<sh->n; i++){
				if(1 != EVP_DigestSignUpdate(sh->ctx[j], sh->messages[j][i], sh->p)){
					printf("/!\\ Error on DigestSignUpdate\n");
				}
				free(sh->messages[j][i]);			
			}
			free(sh->messages[j]);

			if(1 != EVP_DigestSignUpdate(sh->ctx[j], sh->bits[j], sh->n)){
				printf("/!\\ Error on DigestSignUpdate\n");
			}
			if(1 != (ret = EVP_DigestSignFinal(sh->ctx[j], sig, slen))){
				printf("/!\\ Error on DigestSignFinal\n");
				char* error = calloc(256,sizeof(char));
				ERR_error_string(ERR_get_error(), error);
				printf("ERROR : %s\n",error);
				free(error);
			}
			sendto(sh->sockfd_sig, (const uc*)sig, *slen, MSG_CONFIRM, (const struct sockaddr *)&(sh->Sender_sig),sizeof(sh->Sender_sig));
			//printf("Signature for session %d sent:\n",j);
			//tab_display_uc(sig,*slen);
			EVP_MD_CTX_free(sh->ctx[j]);
		}
		free(sh->ctx);
		free(sh->messages);
		free(slen);
		free(sig);
	}
	return 0;
}

int main(int argc, char* argv[]){
	//init random seed
	srand(time(NULL));

	// ####################################
	// ### DECLARATION & INIT VARIABLES ###
	// ####################################
	//Network information
	int port = 12345; // Defaul
	char* ip_sig = atoIP(argc[argv-1]); 
	int port_sig = atoPort(argc[argv-1]);

	// Structure share & parameters
	struct shared sh = {
		.n = 256,
		.p = 512,   // Default values
		.k = 10,
		.mut_mess_rcvd = PTHREAD_MUTEX_INITIALIZER,
		.mess_rcvd = -1
	};
	if((sh.sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0){
		perror("Echec de la creation de la socket");
		exit(EXIT_FAILURE);
	}
	if((sh.sockfd_sig = socket(AF_INET,SOCK_DGRAM,0)) < 0){
		perror("Echec de la creation de la socket");
		exit(EXIT_FAILURE);
	}
	memset(&(sh.Receiver), 0, sizeof(sh.Receiver));
	sh.Receiver.sin_family = AF_INET; //IPv4
	sh.Receiver.sin_addr.s_addr = INADDR_ANY; //any ip
	sh.Receiver.sin_port = htons(port); // port in struct
	if(bind(sh.sockfd,(const struct sockaddr *)&(sh.Receiver),sizeof(sh.Receiver)) < 0){
		perror("Echec de creation du lien");
		exit(EXIT_FAILURE);
	}
	memset(&(sh.Sender), 0, sizeof(sh.Sender));
	memset(&(sh.Sender_sig), 0, sizeof(sh.Sender_sig));
	sh.Sender_sig.sin_family = AF_INET; //Protocole IPv4
	sh.Sender_sig.sin_addr.s_addr = inet_addr(ip_sig); //ip in Receiver struct
	sh.Sender_sig.sin_port = htons(port_sig); // port in Receiver struct


	//Threads
	pthread_t recv_t;
	pthread_t sign_t;

	//Erreurs
	int errno;

	//Getopt
	int option;

	// ##############
	// ### GETOPT ###
	// ##############
	while((option=getopt(argc,argv,"hn:p:k:P:")) != -1){// On indique les lettres des opts et on rajoute
												 // ":" après la lettre si cette opt necessite un
												 // argument
		switch(option){
			case 'h':
				usage_receiver();
				return 0;
			case 'n':
				sh.n = atoi(optarg); //size of one sample
				break;
			case 'p':
				sh.p = atoi(optarg); //size of one packet
				break;
			case 'k':
				sh.k = atoi(optarg); //how many sample to collect before stop
				break;
			case 'P':
				port=atoi(optarg); //port number
				break;
		}
	}

	//Init values depending on n,p,k
	sh.ctx = calloc(sh.k,sizeof(EVP_MD_CTX_new()));
	sh.bits = calloc(sh.k,sizeof(uc*));
	sh.messages = calloc(sh.k, sizeof(uc**));
	for(int j=0; j<sh.k; j++){
		sh.bits[j] = random_bits(sh.n);
		sh.messages[j] = calloc(sh.n,sizeof(uc*));
		for(int i=0; i<sh.n; i++){
			sh.messages[j][i] = calloc(sh.p,sizeof(uc));
		}
	}

	// Cryptographic data
	EVP_PKEY* privkey;
	FILE * fp;
	fp = fopen ("rsa_sk", "r");
	if (fp == NULL) exit (1);
	privkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL); //get sk from sk file
	fclose (fp);
	if (privkey == NULL) {
		ERR_print_errors_fp(stderr);
		exit (1);
	}
	for(int j=0; j<sh.k; j++){
		if(!(sh.ctx[j] = EVP_MD_CTX_create())){
			printf("/!\\ ctx badly created\n");
		}
		if(1 != EVP_DigestSignInit(sh.ctx[j],NULL,EVP_sha256(),NULL,privkey)){
			printf("/!\\ ctx bad Init\n");
		}
	}
	EVP_PKEY_free(privkey);
	free(ip_sig);

	pthread_create(&recv_t,NULL,recving,&sh);
	pthread_create(&sign_t,NULL,signing,&sh);

	pthread_join(recv_t,NULL);
	pthread_join(sign_t,NULL);

	return 0;
}

/*

*/
