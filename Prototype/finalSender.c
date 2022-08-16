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

	unsigned long** rtts; // time samples
	int thresh;
	EVP_MD_CTX** ctx; //context for storage of hash values
	uc** packets;
	uc** bits;

	int sockfd; //sock file descriptor
	struct sockaddr_in Receiver;
	struct sockaddr_in Sender;
	int sockfd_sig;
	struct sockaddr_in Receiver_sig;
	struct sockaddr_in Sender_sig;
	unsigned int slp;

	pthread_mutex_t mut_mess_done;
	pthread_cond_t cond_mess_done;
	int mess_done;
	pthread_mutex_t mut_hashdone;
	pthread_cond_t cond_hashdone;
	int hashdone;
	pthread_mutex_t mut_bits_rcvd;
	pthread_cond_t cond_bits_rcvd;
	int bits_rcvd;
	pthread_mutex_t mut_full_hashdone;
	pthread_cond_t cond_full_hashdone;
	int full_hashdone;
}; // All the variables common to the different Threads

static void* sending(void* p){
	if(p != NULL){
		struct shared* sh = p;
		
		//local
		unsigned long ts = 0;

		for(int j=0; j<sh->k; j++){
			printf("session %d\n",j);
			sh->packets = (uc **)createCharTabCWithTag(sh->n,sh->p,j); //packets = [tag,mess] ~ tag = 2bytes
			sh->mess_done = 1;
			pthread_cond_signal(&sh->cond_mess_done);
			for(int i=0; i<sh->n; i++){
				ts=timestamp();
				sendto(sh->sockfd,(const uc*)sh->packets[i],sh->p+4,MSG_CONFIRM,(const struct sockaddr *)&(sh->Receiver),sizeof(sh->Receiver));
				//printf("  message[%d] envoyé\n",i);
				sh->rtts[j][i] = ts;
				usleep(sh->slp);			
			}
			usleep(sh->slp);
			if(sh->hashdone < j){
				pthread_mutex_lock(&sh->mut_hashdone);
				pthread_cond_wait(&sh->cond_hashdone, &sh->mut_hashdone); //Attendre que hash soti done
				pthread_mutex_unlock(&sh->mut_hashdone);
			}
			for(int i=0; i<sh->n; i++){
				free(sh->packets[i]);
			}
			free(sh->packets);
		}
	}
	printf("TASK send done\n");
	return 0;
}

static void* recving(void* p){
	if(p != NULL){
		struct shared* sh = p;

		//local
		unsigned long ts = 0 ;
		int bufsize; //taille du message reçu (logiquement c'est toujours 3)
		socklen_t addrlen = sizeof(sh->Receiver);
		uc bit; //le random bit reçu (1 uc)p->tag_2b=calloc(2,sizeof(uc));
		int itag_k = 0;
		int itag_n = 0;
		uc* buffer = calloc(MAXLINE,sizeof(uc)); //le buf contenant le message reçu
		uc* uctag_k = calloc(2,sizeof(uc));
		uc* uctag_n = calloc(2,sizeof(uc));

		for(int j=0; j<sh->k; j++){
			for(int i=0; i<sh->n; i++){
				bufsize = recvfrom(sh->sockfd, (uc *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &(sh->Receiver),&addrlen);
				buffer[bufsize]=0;
				//tab_display_uc(buffer,bufsize);
				ts = timestamp();
				bit_and_tag(uctag_k,uctag_n,&bit,buffer);
				itag_k = uc2btoi(uctag_k);
				itag_n = uc2btoi(uctag_n);
				//printf("|tag: (%d,%d)|\n",itag_k,itag_n);
				sh->rtts[itag_k][itag_n] = ts - sh->rtts[itag_k][itag_n];
				sh->bits[itag_k][itag_n] = bit;
			}
			sh->bits_rcvd = j;
			pthread_cond_signal(&sh->cond_bits_rcvd);
		}
		free(buffer);
		free(uctag_k);
		free(uctag_n);
	}
	printf("TASK recv done\n");
	return 0;
}

static void* h_mess(void* p){
	unsigned long ts1 = 0;
	unsigned long ts2 = 0;
	if(p != NULL){
		struct shared* sh = p;
		uc* mess;
		
		for(int j=0; j<sh->k; j++){
			if(sh->mess_done == 0){	
				pthread_mutex_lock(&sh->mut_mess_done);
				pthread_cond_wait(&sh->cond_mess_done, &sh->mut_mess_done);
				pthread_mutex_unlock(&sh->mut_mess_done);
			}
			sh->mess_done = 0;
			ts1=timestamp();
			for(int i=0; i<sh->n; i++){
				//mess_and_tag(sh->n,NULL,NULL,mess,sh->packets[j]);
				mess = extract_string(4,sh->p+4,sh->packets[i],sh->p+4);
				if(1 != EVP_DigestVerifyUpdate(sh->ctx[j], mess, sh->p)){
					printf("/!\\ Error on DigestVerifyUpdate\n");
				}
				free(mess);
			}
			ts2=timestamp();
			printf("### Session %d: h_mess over in %f ms ###\n", j,(double)(ts2-ts1)/1000);
			sh->hashdone = j;
			pthread_cond_signal(&sh->cond_hashdone);
		}
	}
	printf("TASK h_mess done\n");
	return 0;
}

static void* h_bit_decide(void* p){
	if(p != NULL){
		struct shared* sh = p;

		//local
		char result;
		char** str_res = calloc(2,sizeof(uc*));
		str_res[0] = "Accepted\n";
		str_res[1] = "Rejected\n";
		unsigned long ts1 = 0;
		unsigned long ts2 = 0;

		for(int j=0; j<sh->k; j++){
			if(sh->bits_rcvd<j){
				pthread_mutex_lock(&sh->mut_bits_rcvd);
				pthread_cond_wait(&sh->cond_bits_rcvd, &sh->mut_bits_rcvd);
				pthread_mutex_unlock(&sh->mut_bits_rcvd);
			}
			ts1=timestamp();
			if(1 != EVP_DigestVerifyUpdate(sh->ctx[j], sh->bits[j], sh->n)){
				printf("/!\\ Error on DigestVerifyUpdate\n");
			}
			ts2=timestamp();
			sh->full_hashdone = j;
			printf("### Session %d: h_bits done in %f ms ###\n",j, (double)(ts2-ts1)/1000);
			pthread_cond_signal(&sh->cond_full_hashdone);
			// Decision Functionj
			printf("Decide on sample %d\n",j);
			result = Decide(sh->rtts[j],sh->n,sh->thresh,0.2);
			if(result == 1){ //Sample is suspicious
				printf("   ALERT: Bad Times !!!\n");
				printf("RTTs array:\n");
				tab_display_ul(sh->rtts[j],sh->n);
			}
			printf("   Decision Function call on sample %d: %s\n", j,str_res[(int)result]);
			free(sh->bits[j]);
			free(sh->rtts[j]);
		}
		free(sh->rtts);
		// free(str_res[0]);
		// free(str_res[1]);
		free(str_res);
	}
	printf("TASK h_bit_decide done\n");
	return 0;
}

static void* verif(void* p){
	if(p != NULL){
		struct shared* sh = p;

		//local
		socklen_t addrlen = sizeof(sh->Receiver_sig);
		int bufsize;
		uc* buffer = calloc(MAXLINE,sizeof(uc));
		int res;
		for(int j=0; j<sh->k; j++){
			bufsize = recvfrom(sh->sockfd_sig, (uc *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &(sh->Receiver_sig),&addrlen);
			printf("Signature for session %d received:\n",j);
			tab_display_uc(buffer,bufsize);
			if(sh->full_hashdone < j){
				pthread_mutex_lock(&sh->mut_full_hashdone);
				pthread_cond_wait(&sh->cond_full_hashdone, &sh->mut_full_hashdone);
				pthread_mutex_unlock(&sh->mut_full_hashdone);
			}
			if(1 != (res = EVP_DigestVerifyFinal(sh->ctx[j],buffer,bufsize))){
				printf("/!\\ Bad Signature[%d]\n",j);
			}
			else printf("Signature[%d]: Ok\n",j);
			EVP_MD_CTX_free(sh->ctx[j]);
		}
		free(sh->ctx);
		free(buffer);
	}
	return 0;
}
// LAST ONE TO MAKE, THEN RECEIVER, THEN NEW PORT FORWARDING RULES THEN DONE

int main(int argc, char* argv[]){
	//init random seed
	srand(time(NULL));
	
	// ####################################
	// ### DECLARATION & INIT VARIABLES ###
	// ####################################
	// Network information
	char* ip = atoIP(argv[argc-1]);
	int port = atoPort(argv[argc-1]);
	int port_sig = 23456;

	//Structure shared & parameters
	struct shared sh = {
		.n = 256,
		.p = 512,   // Default values
		.k = 10,
		.slp = 0,
		.mut_mess_done = PTHREAD_MUTEX_INITIALIZER,
		.mut_hashdone = PTHREAD_MUTEX_INITIALIZER,
		.mut_bits_rcvd = PTHREAD_MUTEX_INITIALIZER,
		.mut_full_hashdone = PTHREAD_MUTEX_INITIALIZER,
		.hashdone = -1,
		.mess_done = 0,
		.bits_rcvd= -1,
		.full_hashdone = -1
	};
	if((sh.sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0){
		perror("Echec de la creation de la socket");
		exit(EXIT_FAILURE);
	}
	if((sh.sockfd_sig = socket(AF_INET,SOCK_DGRAM,0)) < 0){
		perror("Echec de la creation de la socket");
		exit(EXIT_FAILURE);
	}
	memset(&(sh.Receiver),0,sizeof(sh.Receiver));
	sh.Receiver.sin_family = AF_INET; //Protocole IPv4
	sh.Receiver.sin_addr.s_addr = inet_addr(ip); //ip in Receiver struct
	sh.Receiver.sin_port = htons(port); // port in Receiver struct
	memset(&(sh.Receiver_sig),0,sizeof(sh.Receiver_sig));
	/*sh.Receiver_sig.sin_family = AF_INET; //Protocole IPv4
	sh.Receiver_sig.sin_addr.s_addr = INADDR_ANY; //ip in Receiver struct
	sh.Receiver_sig.sin_port = htons(port_sig); // port in Receiver struct*/
	memset(&(sh.Sender_sig),0,sizeof(sh.Sender_sig));
	sh.Sender_sig.sin_family = AF_INET; //Protocole IPv4
	sh.Sender_sig.sin_addr.s_addr = INADDR_ANY; //ip in Receiver struct
	sh.Sender_sig.sin_port = htons(port_sig); // port in Receiver struct
	if(bind(sh.sockfd_sig,(const struct sockaddr *)&(sh.Sender_sig),sizeof(sh.Sender_sig)) < 0){
		perror("Link creation failed");
		exit(EXIT_FAILURE);
	}
	free(ip);

	//Threads
	pthread_t send_t;
	pthread_t recv_t;
	pthread_t h_mess_t;
	pthread_t h_bit_decide_t;
	pthread_t verif_t;

	//Decision Function parameters
	FILE* f_ref;
	char* refname = calloc(MAXLINE,sizeof(char));
	int refsize;
	int irefsize;
	int* ref;
	char* ref_str;

	//Erreurs
	int errno; //On ne s'en sert pas en l'état mais il faudrait

	//Getopt
	int option;

	//Switches
	//char switch_Time;

	// ##############
	// ### GETOPT ###
	// ##############
	while((option=getopt(argc,argv,"hk:p:n:s:r:TS")) != -1){// On indique les lettres des opts et on rajoute
												 // ":" après la lettre si cette opt necessite un
												 // argument
		switch(option){
			case 'h': // help
				usage_sender();
				return 0;
			case 'n':
				sh.n = atoi(optarg); //taille d'un sample
				break;
			case 'p':
				sh.p = atoi(optarg); //taille des paquets
				break;
			case 'k':
				sh.k = atoi(optarg); //nombre de sample à récolter
				break;
			case 's':
				sh.slp = (unsigned int) atoi(optarg);
				break;
			case 'r':
				for(int i=0; i<strlen(optarg);i++){
					refname[i]=optarg[i];
				}
				refname[strlen(optarg)]='\0';
				break;
			// case 'T':
			// 	switch_Time=1;
			// 	break;
		}
	}
printf("test1\n");
	//Init values depending on n,p,k
	sh.rtts = calloc(sh.k,sizeof(unsigned long*));
	sh.ctx = calloc(sh.k,sizeof(EVP_MD_CTX_new()));
	sh.bits = calloc(sh.k,sizeof(uc*));
	for(int j=0; j<sh.k; j++){
		sh.rtts[j] = calloc(sh.n,sizeof(unsigned long));
		sh.bits[j] = calloc(sh.n,sizeof(uc));
	}
printf("t2\n");

	//Cryptographic data
	EVP_PKEY* pubkey;
	FILE * fp;
	fp = fopen ("pk.pem", "r");
	if (fp == NULL) exit (1);
	pubkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL); //get pk from pk file
	fclose (fp);
	if (pubkey == NULL) {
		ERR_print_errors_fp(stderr);
		exit (1);
	}
	for(int j=0; j<sh.k; j++){
		if(!(sh.ctx[j] = EVP_MD_CTX_create())){
			printf("/!\\ ctx badly created\n");
		}
		if(1 != EVP_DigestVerifyInit(sh.ctx[j],NULL,EVP_sha256(),NULL,pubkey)){
			printf("/!\\ ctx bad Init\n");
		}
	}
	EVP_PKEY_free(pubkey);
printf("t3\n");
	//Threshold generation
	f_ref=fopen(refname,"r");
	refsize = get_file_size(f_ref);
	ref_str = calloc(refsize,sizeof(char));

	irefsize=get_line(ref_str,refsize,f_ref);
	fclose(f_ref);
	ref = calloc(irefsize,sizeof(int));
	tab_from_line(ref,irefsize,ref_str);
	free(ref_str);
	qsort(ref,irefsize,sizeof(int),compare);
	sh.thresh=ref[(int)(0.8*(float)irefsize)+1]+15000; //thresh is set as the 8th decile of ref + a marge of 10 ms
	free(ref);
	free(refname);

	printf("Protocol Parameters:");
	printf("   n = %d\n",sh.n);
	printf("   p = %d\n",sh.p);
	printf("   k = %d\n",sh.k);
	printf("Communicating with %s\n", ip);
	printf("Decision Function's threshold : %d\n",sh.thresh);

	pthread_create(&send_t,NULL,sending,&sh);
	pthread_create(&recv_t,NULL,recving,&sh);
	pthread_create(&h_mess_t,NULL,h_mess,&sh);
	pthread_create(&h_bit_decide_t,NULL,h_bit_decide,&sh);
	pthread_create(&verif_t,NULL,verif,&sh);

	pthread_join(send_t,NULL);
	pthread_join(recv_t,NULL);
	pthread_join(h_mess_t,NULL);
	pthread_join(h_bit_decide_t,NULL);
	pthread_join(verif_t,NULL);



	return 0;
}

//Encore des trucs à free ici, notamment les ctx