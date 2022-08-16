#include <stdio.h>

//functions_verify.c
extern void sha256_uc(char*, unsigned char*);
extern void Hash(unsigned char**, unsigned char*, int, int, unsigned char*);
extern unsigned char* Sign(unsigned char*,int*);
extern int Verify(unsigned char*,int, unsigned char*);
extern int get_file_size(FILE*);
extern int get_line(char*,int,FILE*);
extern void tab_from_line(int*, int, char*);
extern char Decide(unsigned long*,int, int, float);
extern int compare(const void*, const void*);
extern unsigned long timestamp(void);
extern char string_comp(char*, char*);

//functions_string.c
extern void random_string(char*, int);
extern unsigned char* random_bits(int);
extern unsigned char** createCharTabC(int, int);
extern unsigned char** createCharTabCWithTag(int, int, int);
extern unsigned char* extract_string(int, int, unsigned char*,int);
extern void mess_and_tag(int,unsigned char*,unsigned char*, unsigned char***,unsigned char*);
extern void bit_and_tag(unsigned char*,unsigned char*,unsigned char*, unsigned char*);
extern void add_mess(unsigned char**, int, unsigned char*, int);

//functions_args_conv.c
extern unsigned char* itouc2b(int);
extern int uc2btoi(unsigned char*);
extern char* atoIP(char*);
extern int atoPort(char*);

//functions_others.c
extern void usage_sender(void);
extern void usage_receiver(void);
extern void tab_display_ul(unsigned long *tab, int tablen);
extern void tab_display_uc(unsigned char*, int);
