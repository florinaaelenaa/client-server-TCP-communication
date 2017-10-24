#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

#include <fcntl.h>
#include <sys/types.h>					//mkdir - create directories
#include <sys/stat.h>

#include <dirent.h>						//print files from directory

#include <unistd.h>						//print the pathname of directory
#include <errno.h>


#define MAX_CLIENTS				15
#define BUFLEN 					4096
#define FOREVER 		  		while(1)

		

#define PRIVATE 				 1
#define SHARED 					 2

#define LOGGED_USER				 1		
#define UNLOGGED_USER	 		-1		// Clientul nu e autentificat
#define SESSION_ALREADY_OPEN 	-2		// Sesiune deja deschisa 		Explicatie: Nu poti deschide 2 sesiuni in cadrul aceluiasi client
#define WRONG_PASS				-3		// User/parola gresita
#define UNEXISTING_FILE			-4		// Fisier inexistent
#define FORBIDDEN_DOWNLOAD		-5		// Descarcare interzisa			Explicatie: Nu ai dreptul de a descarca acest fisier
#define FILE_ALREADY_SHARED		-6		// Fisier deja partajat
#define FILE_ALREADY_PRIVATE	-7		// Fisier deja privat
#define BRUTE_FORCE				-8		// Brute-force detectat
#define EXISTING_FILE			-9		// Fiser existent pe server 	Explicatie: Fisierul exista deja pe server
#define	TRANSFERING_FILE		-10		// Fisier in transfer 	 		Explicatie: Fisier aflat momentan in transfer
#define UNEXISTING_USER			-11		// Utilizator inexistent



typedef struct 
{
	char file_name[100];
	int type;
	unsigned long size;
}USER_FILES;

typedef struct
{
	char *name;
	char *password;
	int number;
	int no_socket;
	int PID;
	USER_FILES *file;
}USERS;


typedef struct 
{
	int file_descriptor;
	unsigned long offset;

}TRANSFER;

