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

struct shared{
	unsigned long* rtts; // Les données partagées entre le reader et le writer
	int n; //taille de rtts
	int p; //taille des paquets
	int sockfd;
	socklen_t respLen;
	struct sockaddr_in Receiver;
	unsigned char** packets;
	unsigned char* bits;
	unsigned int slp;
	pthread_mutex_t mut; // cette variable va permettre de protéger une tâche d'être
	                     // interrompue par un autre
};

struct data_s{
	unsigned long ts;
	struct shared* psh;
};

struct data_r{
	unsigned long ts;
	uc* buffer;
	uc* tag_2b;
	struct shared* psh;
};


static void* task_send(void* p){
	if(p!=NULL){
		struct data_s* p_data=p;
		p_data->psh->rtts=calloc(p_data->psh->n,sizeof(unsigned long));
		unsigned long ts=0;

		for(int i=0; i<p_data->psh->n; i++){
			pthread_mutex_lock(&p_data->psh->mut);
			ts=timestamp();
			sendto(p_data->psh->sockfd,(const unsigned char *)p_data->psh->packets[i],p_data->psh->p+2         ,MSG_CONFIRM,(const struct sockaddr *)&(p_data->psh->Receiver),sizeof(p_data->psh->Receiver));
			//     point. vers sockfd   point. vers les paquets              point. vers taille paquet
			pthread_mutex_unlock(&p_data->psh->mut);
			p_data->psh->rtts[i] = ts;
			usleep(p_data->psh->slp);
		}
	}
	return 0;
}

static void* task_recv(void* p){
	if(p!=NULL){
		struct data_r* p_data=p; //la structure contenant les infos importantes (sockfd, sockaddr, n, etc...)
		// Variables locales
		unsigned long ts=0;
		int bufsize; //taille du message reçu (logiquement c'est toujours 3)
		unsigned char bit; //le random bit reçu (1 unsigned char)p->tag_2b=calloc(2,sizeof(unsigned char));
		int tag_i=0; //le même tag sous forme de int
		p_data->buffer=calloc(MAXLINE,sizeof(unsigned char)); //le buf contenant le message reçu
		p_data->tag_2b=calloc(2,sizeof(uc));
		for(int i=0; i<p_data->psh->n; i++){
			bufsize=recvfrom(p_data->psh->sockfd, (unsigned char *)p_data->buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &(p_data->psh->Receiver),&(p_data->psh->respLen));
			pthread_mutex_lock(&p_data->psh->mut);
			ts=timestamp();
			//tab_display_uc(buffer,3);
			p_data->psh->rtts[i] = ts - p_data->psh->rtts[i];
			bit_and_tag(p_data->tag_2b,&bit,p_data->buffer);
			tag_i=uc2btoi(p_data->tag_2b);
			p_data->psh->bits[tag_i]=bit;
			pthread_mutex_unlock(&p_data->psh->mut);
			//printf("   Computing rtt on index %d : %lu\n",i,p_data->psh->rtts[i]);
		}
		free(p_data->buffer);
		free(p_data->tag_2b);
	}
	return 0;
}

// static void* task_verify(void* p){

// }// check on array signatures to be fill and verify (no mutex)

int main(int argc, char* argv[]){
	// #############################
	// ### DECLARATION VARIABLES ###
	// #############################
	//Erreurs
	int errno;

	//Getopt
	int option;

	//Socket
	int sockfd; 
	struct sockaddr_in Receiver;
	socklen_t respLen=sizeof(Receiver);

	//Receiver and Attacker Information
	char* ip;
	int port;
	char* dest;

	//protocol parameters
	int n=256; // taille d'un sample
	int p=512; // taille d'un paquet
	int k=1; // nombre de sample à récolter

	//decision function parameters
	FILE* f_ref;
	char* refname = calloc(MAXLINE,sizeof(char));
	int refsize;
	int irefsize;
	int* ref;
	int thresh=0;
	char result=0;
	char* ref_str;

	//Timestamps
	char switch_Time=0;
	unsigned long tsg1,tsg2,tscrypto1,tscrypto2;
	double total,crypto;
	
	//Cryptographic data
	uc* hash;
	uc* sign;
	int sign_size;

	// init random seed
	srand(time(NULL));

	// other
	unsigned int slp=0;

	// ##############
	// ### GETOPT ###
	// ##############
	while((option=getopt(argc,argv,"hk:p:n:s:r:TS"))!=-1){// On indique les lettres des opts et on rajoute
												 // ":" après la lettre si cette opt necessite un
												 // argument
		switch(option){
			case 'h': // help
				usage_sender();
				return 0;
			case 'k':
				k=atoi(optarg); //nombre de sample à récolter
				break;
			case 'p':
				p=atoi(optarg); //taille des paquets
				break;
			case 'n':
				n=atoi(optarg); //taille d'un sample
				break;
			case 's':
				slp=(unsigned int) atoi(optarg);
				break;
			case 'r':
				for(int i=0; i<strlen(optarg);i++){
					refname[i]=optarg[i];
				}
				refname[strlen(optarg)]='\0';
				break;
			case 'T':
				switch_Time=1;
				break;
		}
	}

	//Génération du seuil
	f_ref=fopen(refname,"r");
	refsize = get_file_size(f_ref);
	ref_str = calloc(refsize,sizeof(char));

	irefsize=get_line(ref_str,refsize,f_ref);
	fclose(f_ref);
	ref = calloc(irefsize,sizeof(int));
	tab_from_line(ref,irefsize,ref_str);
	free(ref_str);
	qsort(ref,irefsize,sizeof(int),compare);
	thresh=ref[(int)(0.8*(float)irefsize)+1]+15000; //thresh is set as the 8th decile of ref + a marge of 10 ms
	printf("Threshold value: %d\n", thresh);
	free(ref);

	// Network information
	ip=atoIP(argv[argc-1]);
	port=atoPort(argv[argc-1]);
	if((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0){
		perror("Echec de la creation de la socket");
		exit(EXIT_FAILURE);
	}
	memset(&Receiver,0,sizeof(Receiver)); //Init à 0 de la struct
	Receiver.sin_family=AF_INET;//Protocole IPv4
	Receiver.sin_addr.s_addr=inet_addr(ip);//sin_addr est une struct in addr{unsigned long s_addr}, 
													 //elle ne contient qu'un unsigned long
	Receiver.sin_port=htons(port); //port

	//Declaration of Threads and structures
	pthread_t sender;
	pthread_t receiver;

	struct shared sh={
		//.rtts=calloc(n,sizeof(unsigned long)),
		.sockfd=sockfd,
		.Receiver=Receiver,
		//.packets=(unsigned char **)createCharTabCWithTag(n,p), //packets = [tag,mess] ~ tag = 2bytes
		.respLen=sizeof(Receiver),
		.n=n,
		.p=p,
		.slp=slp
	};
	
	struct data_s d_sender={
		.ts=0,
		.psh=&sh,
	};

	struct data_r d_receiver={
		.ts=0,
		.psh=&sh,
	};
	if(switch_Time) tsg1=timestamp();
	for(int nb_samp=0; nb_samp<k; nb_samp++){
		printf("### TOUR %d ###\n",nb_samp);
		// Envoi des données
		sh.packets=(uc **)createCharTabCWithTag(n,p); //packets = [tag,mess] ~ tag = 2bytes
		sh.bits=calloc(n,sizeof(uc));
		
		pthread_create(&sender,NULL,task_send,&d_sender);
		pthread_create(&receiver,NULL,task_recv,&d_receiver);

		pthread_join(sender,NULL);
		pthread_join(receiver,NULL);

		tscrypto1=timestamp();
		// Hash
		Hash(sh.packets,sh.bits,sh.n,sh.p,hash);
		//hash=HashViewSender(sh.packets,sh.bits,sh.n,sh.p);
		printf("   %d packets sent...\n",n);
		sign = calloc(MAXLINE,sizeof(uc));
		sign_size=recvfrom(sockfd, (unsigned char *)sign, MAXLINE,MSG_WAITALL, (struct sockaddr *) &Receiver,&respLen);
		sign[sign_size]='\0'; // Réception de la signature

		// Vérification de la signature
		if(Verify(sign,256,hash)){
			printf("   Sign: VALID\n");
		}
		else{
			printf("   ALERT: Bad Signature !!!");
			return -1;
		}
		// Decision Function
		result = Decide(sh.rtts,sh.n,thresh,0.2);
		if(result==1){ //Sample is suspicious
			printf("   ALERT: Bad Times !!!\n");
			printf("Tableau des RTTs:\n");
			for(int i=0; i<sh.n; i++){
				printf("%lu -",sh.rtts[i]);
			}
			printf("\n");
			//return -1;
		}
		printf("   Decision de la fonction : %d\n",result);
		//free everything
		free(hash);
		free(sign);
		for(int i=0; i<n; i++){
			free(sh.packets[i]);
		}
		free(sh.rtts);
		free(sh.packets);
		free(sh.bits);
		tscrypto2=timestamp();
		crypto=(double)(tscrypto2-tscrypto1)/1000000;
		printf("   Time(Hash+Sign+Decide): %lfs\n\n",crypto);
	}
	if(switch_Time) tsg2=timestamp();
	total=(double)(tsg2-tsg1)/1000000;
	printf("Temps de l'envoi: %lfs\n",total);
	free(ip);
	return 0;
}
/*
task_send:
	while(switchPack!=0){ //on ne continue que si switchPack vaut 0
		pass
	}
	packets=[m_0, ... , m_n]
	switchPack=1
	for i =1..n:
		send()

tesk_recv:
	for i=1..n
		recv()
		bit -> bits
	switchBit=1

task_verif:
	while(switchPack != 1){
		pass
	}
	packets -> temp for hash
	switchPack=0
	while(switchBit !=1){
		pass
	}
	bits -> bits_temp_for_hash
	switchBit=0

*/