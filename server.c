#include "utilities.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int login(USERS *user, int no_users, int i, char *user_aux, char *pass_aux)
{
	// unde creezi useri noi? in main
	int j, n, ok = 0;
	char aux[BUFLEN];

	for(j = 0; j < no_users; ++j)
	{
		if(strncmp(user[j].name, user_aux, strlen(user[j].name)) == 0)
		{
			if(strncmp(user[j].password, pass_aux, strlen(user[j].password)) == 0)
			{
				//salvam socket-ul pe care s-a facut autentificarea
				user[j].no_socket = i;

				//salvam PID-ul procesului pe care e logat user-ul												
				user[j].PID = getpid();

				// resetam valoarea contorului de brutforce												
				ok = 0;	

				//returneaza clientului ca a reusit autentificare
				sprintf(aux, "%d", LOGGED_USER);									
				send(i, aux, strlen(aux), 0);
				break;  
			}
			else
			{
				if(n == j)
				{
					ok++;
					if(ok == 3)
					{
						//resetam valoarea contorului de brutforce pentru ca acesta returneaza warningul clientului
						ok = 0;														
						printf("Brut Force detected! \n");
						sprintf(aux, "%d", BRUTE_FORCE);
						send(i, aux, strlen(aux), 0);
						return -1;
						
					}
					n = j;
				}
				else
					// s-a incercat autentificarea pe un alt user, dar cu o parola gresita, deci se reseteaza ok-ul
					ok = 0; 														
			}
		}
	}
	if(j == no_users)
	{
			printf("Wrong user/password\n");
			sprintf(aux, "%d", WRONG_PASS);
			send(i, aux, strlen(aux), 0);	
	}
	return 0;
}

char* copy_string(char *user_list, USERS usr, int k)
{
	char aux[BUFLEN];
	memset(aux, 0, BUFLEN);
	strcat(user_list, usr.file[k].file_name);
	strcat(user_list, " ");
	sprintf(aux, "%lu", usr.file[k].size);
	strcat(user_list, aux); 
	strcat(user_list, "bytes ");

	return user_list;
}

void get_file_list(USERS *user, int no_users, int i, char *user_aux)
{
	int k, j;
	char *user_list;
	
	for(j = 0; j < no_users; ++j)
		if(strncmp(user[j].name, user_aux, strlen(user[j].name)) == 0)
			break;
							
	if(j == no_users)
	{
		user_list = malloc(sizeof(int)*2);
		memset(user_list, 0, 4);
		sprintf(user_list, "%d", UNEXISTING_USER);
		send(i, user_list, strlen(user_list) + 1, 0);
	}
	else
	{
		user_list = malloc((sizeof(char) * ((strlen(user[j].file[0].file_name) + strlen("PRIVATE") + strlen("bytes ")) + 
							sizeof(user[j].file[0].size) + 
							sizeof(user[j].file[0].type))));

		memset(user_list, 0, (strlen(user[j].file[0].file_name) + 
							  strlen("PRIVATE") + 
							  strlen("bytes ") +
							  sizeof(user[j].file[0].size) + 
							  sizeof(user[j].file[0].type)));

		if(user[j].file[0].type == PRIVATE)
		{
			user_list = copy_string(user_list, user[j], 0);
			strcat(user_list, "PRIVATE");		
		}
		else
		{
			user_list = copy_string(user_list, user[j], 0);
			strcat(user_list, "SHARED");
		}
		strcat(user_list, "\n");
		
		for(k = 1; k < user[j].number; ++k)
		{
			user_list = realloc(user_list, (sizeof(char) * ((strlen(user[j].file[k].file_name) + strlen("PRIVATE") + strlen("bytes ")) + 
											sizeof(user[j].file[k].size) + 
											sizeof(user[j].file[k].type)))*(k+1));

			if(user[j].file[k].type == PRIVATE)
			{
				user_list = copy_string(user_list, user[j], k);
				strcat(user_list, "PRIVATE");		
			}
			else
			{
				user_list = copy_string(user_list, user[j], k);
				strcat(user_list, "SHARED");
			}
			strcat(user_list, "\n");
		}
		
		send(i, user_list, strlen(user_list) + 1, 0);
		free(user_list);
			
	}
}

void get_user_list(USERS *user, int no_users, int i)
{
	int j;
	char *user_list;

	unsigned int size = strlen(user[0].name) + 2; 
	user_list = malloc(sizeof(char) * size); 

	memset(user_list, 0, strlen(user[0].name) + 2);
	strcat(user_list, user[0].name); 
	strcat(user_list, "\n"); 
	
	for(j = 1; j < no_users; ++j)
	{
		size += strlen(user[j].name) + 1;
		user_list = realloc(user_list, sizeof(char) * size); 
		strcat(user_list, user[j].name);
		strcat(user_list, "\n");
	}

	send(i, user_list, strlen(user_list) + 1, 0); 
	free(user_list);
}

void share(USERS *user, int no_users, int i, char *command )
{
	char buffer[BUFLEN];
	int j, l;

	for(j = 0; j < no_users; ++j)
		if( i == user[j].no_socket)
		{
			for(l = 0; l < user[j].number; ++l)
				if(strncmp(user[j].file[l].file_name, command, strlen(command)-1) == 0)
				{	
					if(user[j].file[l].type == SHARED)
					{
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%d", FILE_ALREADY_SHARED);
						send(i, buffer, strlen(buffer) + 1, 0);
					}
					else
					{
						user[j].file[l].type = SHARED;
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%s", "converted");
						send(i, buffer, strlen(buffer) + 1, 0);
					}
					break;	
				}
			if(l == user[j].number)
			{
				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%d", UNEXISTING_FILE);
				send(i, buffer, strlen(buffer) + 1, 0);
			}
		}
}

void unshare(USERS *user, int no_users, int i, char *command)
{
	int j, l;
	char buffer[BUFLEN];

	for(j = 0; j < no_users; ++j)
		if( i == user[j].no_socket)
		{
			for(l = 0; l < user[j].number; ++l)
				if(strncmp(user[j].file[l].file_name, command, strlen(command)-1) == 0)
				{	
					if(user[j].file[l].type == PRIVATE)
					{
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%d", FILE_ALREADY_PRIVATE);
						send(i, buffer, strlen(buffer) + 1, 0);
					}
					else
						{
							user[j].file[l].type = PRIVATE;
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "%s", "converted");
							send(i, buffer, strlen(buffer) + 1, 0);
						}
					break;	
				}
			if(l == user[j].number)
			{
				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%d", UNEXISTING_FILE);
				send(i, buffer, strlen(buffer) + 1, 0);
			}
		}
}

void delete(USERS *user, int no_users, int i, char *command)
{
	int j, k, l;
	char aux[BUFLEN], name_aux[BUFLEN], buffer[BUFLEN];

	for(j = 0; j < no_users; ++j)
		if( i == user[j].no_socket)
		{
			for(k = 0; k < user[j].number; ++k)
				if(strncmp(user[j].file[k].file_name, command, strlen(user[j].file[k].file_name)) == 0)
				{
					memset(aux, 0,  BUFLEN);
					memset(name_aux, 0,  BUFLEN);
					if (getcwd(name_aux, sizeof(name_aux)) == NULL)
						perror("getcwd() error");
					strcat(name_aux,"/");
					strcat(name_aux, user[j].name);
					strcat(name_aux,"/");
					strcat(name_aux, user[j].file[k].file_name);	
					sprintf(aux,"%s%s", "rm ", name_aux);
					
					user[j].number--;
					
					system(aux);
					
					for(l = k; l < user[j].number; ++l)
						user[j].file[l] = user[j].file[l+1];
					sprintf(buffer, "%s", "deleted");
					send(i, buffer, strlen(buffer) + 1, 0);
					break;
				}
			if(k == user[j].number)
			{
				memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%d", UNEXISTING_FILE);
				send(i, buffer, strlen(buffer) + 1, 0);
			}

		}
}

TRANSFER* upload(USERS *user, int no_users, int i, char *file_aux, TRANSFER *files, char buffer2[])
{
	int  destination;
	char destination_path[100];
	char buffer[BUFLEN];
	char name_aux[BUFLEN];
	int n, k, j; 

	FILE *in1;

	if(files[i].offset == 0)
	{
		for(j = 0; j < no_users; ++j)
			if(i == user[j].no_socket)
			{
				for(k = 0; k < user[j].number; ++k)
				{
					if(strncmp(user[j].file[k].file_name, file_aux, strlen(user[j].file[k].file_name)) == 0)
					{
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%d", EXISTING_FILE);
						send(i, buffer, strlen(buffer) + 1, 0);
						goto END;
					}
				}
	    		send(i, buffer, 1, 0);	
	    		memset(destination_path, 0,  100);
				memset(name_aux, 0,  BUFLEN);
				if (getcwd(name_aux, sizeof(name_aux)) == NULL)
					perror("getcwd() error");
				strcat(name_aux,"/");
				
				strcat(name_aux, user[j].name);								
				strcat(destination_path, name_aux);
				strcat(destination_path,"/");
				strncat(destination_path, file_aux, strlen(file_aux)-1);

	    		destination = open(destination_path, O_CREAT | O_WRONLY, 0777);
	    		lseek(destination, 0, SEEK_SET);

	    		memset(buffer, 0, BUFLEN);
		    	recv(i, buffer, sizeof(buffer), 0);
		    	printf("recv buf:::\n %s\n", buffer);
		    	
		        k = write(destination, buffer, strlen(buffer));
		        if (k == -1) 
		        {
		            printf("Error writing to file.\n");
		            exit(1);
		        }

	    		break;
	    	}
	    }
	    else 
	    	{
	    		destination = files[i].file_descriptor;
	    		lseek(destination, files[i].offset, SEEK_SET);
	    		memset(buffer, 0, BUFLEN);
	    		strcpy(buffer, buffer2);
	    		k = write(destination, buffer, strlen(buffer));
		        if (k == -1) 
		        {
		            printf("Error writing to file.\n");
		            exit(1);
		        }
	    	}
		
		
	    //memmorez transferul de fisier intr-un vector
    	if( (strlen(buffer) > BUFLEN) && (files[i].file_descriptor == 0) )
    	{
    		files[i].file_descriptor = destination;
    		files[i].offset += BUFLEN; 
    	}
    	else 
    		if(strlen(buffer) > BUFLEN)
    			files[i].offset += BUFLEN;
    	else
    		if(strlen(buffer) < BUFLEN)
    		{
    			// am terminat de facut upload si resetez offsetul la 0 pe socketul i
    			files[i].offset = 0;    				
    			files[i].file_descriptor = 0;
    			close(destination);

    			in1 = fopen(destination_path, "r");
			    fseek(in1, 0, SEEK_END);

			    for(j = 0; j < no_users; ++j)
					if(i == user[j].no_socket)
					{
					    user[j].number ++;
					    user[j].file = realloc(user[j].file, sizeof(USER_FILES) * user[j].number);
						strcpy(user[j].file[user[j].number - 1].file_name, file_aux);
						user[j].file[user[j].number - 1].type = PRIVATE;
						user[j].file[user[j].number - 1].size = ftell(in1);
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%s%s%s%lu%s", "Upload finished: ", user[j].file[user[j].number - 1].file_name,
													   " - ", user[j].file[user[j].number - 1].size, "bytes" );
					}

				fclose(in1);

				send(i, buffer, strlen(buffer) + 1, 0);
    		}



END:	return files;

}

void download(USERS *user, int no_users, int i, char file_aux[], char user_aux[])
{
	int  destination, source;
	char destination_path[100], source_path[100];
	char buffer[BUFLEN], aux[BUFLEN];
	char name_aux[BUFLEN];
	int n, k, j, ok = 0; 

	FILE *in1;

	if (getcwd(source_path, sizeof(source_path)) == NULL)
	     perror("getcwd() error");  
							strcat(source_path, "/");

	for(j = 0; j < no_users; ++j)
    	if(user[j].no_socket == i)
    	{
			strcat(destination_path, source_path);
    		strcat(destination_path, user[j].name);	
    		break;						
    	}
    strcat(source_path, user_aux);											
    strcat(source_path, "/");
    strcat(destination_path, "/");
    strncat(source_path, file_aux, strlen(file_aux)-1);
    
    memset(aux, 0, BUFLEN);
	sprintf(aux, "%d", user[j].PID);
	strcat(destination_path, aux);
	strcat(destination_path, "_");
    strncat(destination_path, file_aux, strlen(file_aux)-1);

    printf("source_path       %s\n", source_path);
    printf("destination_path  %s\n", destination_path);
    
    for(j = 0; j < no_users; ++j)
    {
    	if(strncmp(user[j].name, user_aux, strlen(user[j].name)) == 0)
    	{
    		for(k = 0; k < user[j].number; ++k)
        		if(strncmp(user[j].file[k].file_name, file_aux, strlen(user[j].file[k].file_name)) == 0)
        		{
        			if(user[j].file[k].type == PRIVATE)
        			{
        				memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%d", FORBIDDEN_DOWNLOAD);
						send(i, buffer, strlen(buffer) + 1, 0);
						goto END;
        			}
        			else
        			{

        				source = open(source_path, O_RDONLY);
					    if(source == -1)
					    	printf("nu se deschide sursa\n");
					    destination = open(destination_path, O_CREAT | O_WRONLY, 0777);
					    if(destination == -1)
					    	printf("nu se deschide destinatia\n");
					   
					    lseek(source, 0, SEEK_SET);
					    lseek(destination, 0, SEEK_SET);

					    do
					    {
					    	memset(buffer, 0, BUFLEN);
					        ok = read(source, buffer, 4096);
					        if (k == -1) 
					        {
					            printf("Error reading file.\n");
					            exit(1);
					        }
					        n = ok;

					        if (n == 0) 
					        	break;

					        ok = write(destination, buffer, n);
					        if (ok == -1) 
					        {
					            printf("Error writing to file.\n");
					            exit(1);
					        }

					    }FOREVER;

					    close(source);
					    close(destination);

					   

					    memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%s%s%s%lu%s", "Download finished: ", user[j].file[k].file_name, " - ", user[j].file[k].size, "bytes" );
						send(i, buffer, strlen(buffer) + 1, 0);
						break;

        			}
        		}

        	if(k == user[j].number)
            {
            	memset(buffer, 0, BUFLEN);
				sprintf(buffer, "%d", UNEXISTING_FILE);
				send(i, buffer, strlen(buffer) + 1, 0);
            }
        }
    }
    
    if(j == no_users)
    {
    	memset(buffer, 0, BUFLEN);
		sprintf(buffer, "%d", UNEXISTING_USER);
		send(i, buffer, strlen(buffer) + 1, 0);
    }

	END: ;
}

int main(int argc, char *argv[])
{


	// --------------> DECLARARI VARIABILE <--------------
	int sockfd, newsockfd, portno, client_len;
	int number_fd, destination;
	
	char buffer[BUFLEN], aux[BUFLEN], aux2[BUFLEN];
	char *p, *command, name_aux[BUFLEN];
	char *user_aux, *pass_aux, *file_aux;		//pentru comenzile clientilor
	char *user_list;							//lista de utilizatori ceruta de client										

	struct sockaddr_in serv_addr, cli_addr;
	
	int i, j, k, l, ok = 0, n;
	
	int no_users;								//nr utilizatori din user_config
	int no_files;								//nr fisire partajate din shared_files
	
	USERS *user;								//vectorul de useri din user_config 
	
	struct timeval timeout;						//timeout-ul setat pentru select()
	timeout.tv_sec = 10;								
	timeout.tv_usec = 0;

	fd_set read_fds;							//multimea de citire folosita in select()
	fd_set tmp_fds;								//multime folosita temporar 
	int fdmax;									//valoare maxima file descriptor din multimea read_fds

	DIR           *d;							//gestionare fisiere dintr-un director ( readdir() )
  	struct dirent *dir;

  	TRANSFER *files;
  	files = (TRANSFER*)malloc(sizeof(TRANSFER) * (MAX_CLIENTS + 1));
  	for(i = 0; i < MAX_CLIENTS + 1; ++i)
	{	
		files[i].file_descriptor = 0;
		files[i].offset = 0;
	}

	FILE *in1, *in2;
	in1 = fopen(argv[2], "r");
	fseek(in1, 0, SEEK_SET);
	in2 = fopen(argv[3], "r");
	fseek(in2, 0, SEEK_SET);

	portno = atoi(argv[1]);


	// --------------> CONFIGURARE USERI <--------------
	fscanf(in1, "%d", &no_users);
	user = (USERS*)malloc(sizeof(USERS) * no_users);
	
	for(i = 0; i < no_users; ++i)
	{
		memset(buffer, 0 , BUFLEN);
		fscanf(in1, "%s", buffer);
		user[i].name = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
		strcpy(user[i].name, buffer);
		//in cazul in care un utilizator din user_config nu are folder de partajare
		mkdir(user[i].name, 0777);													
		memset(buffer, 0 , BUFLEN);
		fscanf(in1, "%s", buffer);
		user[i].password = (char*)malloc(sizeof(char) * (strlen(buffer) + 1));
		strcpy(user[i].password, buffer);

		user[i].number = 0;
		user[i].PID = 0;
	}
	fclose(in1);	



	// --------------> CONFIGURARE FISIERE PARTAJATE <--------------
	fscanf(in2, "%d", &no_files);

	for(i = 0; i < no_files;  ++i)
	{
		memset(buffer, 0 , BUFLEN);
		fscanf(in2, "%s", buffer);

		for(j = 0; j < no_users; ++j)
			if(strncmp(user[j].name, buffer, strlen(user[j].name)) == 0)
				break;

		user[j].number ++;
		
		user[j].file = realloc(user[j].file, sizeof(USER_FILES) * user[j].number);
		memset(name_aux, 0 , 50);
		p = strtok(buffer,":");
		p = strtok(NULL, ":");
		strcpy(name_aux, p);
		strcpy(user[j].file[user[j].number - 1].file_name, name_aux);
		user[j].file[user[j].number - 1].type = SHARED;
		user[j].no_socket = -1;
	}
	fclose(in2);



	// --------------> CONFIGURARE DIRECTOARE + FISIERE EXISTENTE <--------------
	for(i = 0; i < no_users; ++i)
	{
		k = user[i].number;
		memset(name_aux, 0 , BUFLEN);
		//creeaza calea din directorul curent in directorul user-ului
		if (getcwd(name_aux, sizeof(name_aux)) == NULL)									 
			perror("getcwd() error");
		strcat(name_aux,"/");
		strcat(name_aux, user[i].name);	

		//verifica toate fisierele (partajate sau nu) pentru fiecare user in parte
		d = opendir(name_aux);															 
		memset(aux, 0 , BUFLEN);
		strcpy(aux, name_aux);
		strcat(aux,"/");
		if(d)
		{
			while ((dir = readdir(d)) != NULL)
			{	
				
				if( (strcmp(dir->d_name, ".") != 0) && (strcmp(dir->d_name, "..") != 0) ) //vrem sa stabilim si dimensiunea fisierului
				{
					memset(name_aux, 0 , BUFLEN);
					strcpy(name_aux,aux);
					strcat(name_aux, dir->d_name);
					
					in1 = fopen(name_aux, "r");
					fseek(in1, 0L, SEEK_SET);
					fseek(in1, 0L, SEEK_END);

					
					for(j = 0; j < k; ++j)
						if(strncmp(user[i].file[j].file_name, dir->d_name, strlen(dir->d_name)) == 0)
							//daca gaseste in vectorul de fisiere, cel pe care tocmai l-a citit, da break si sare peste el
							break;														
					if(j < k)
						user[i].file[j].size = ftell(in1);

					//daca nu-l gaseste se ajunge la finalul vectorului cu fisiere partajate si il adauga in continuare in vectorul cu fisiere
					if(j == k)															
					{
						user[i].number++;
						user[i].file = realloc(user[i].file, sizeof(USER_FILES) * user[i].number);
						strcpy(user[i].file[user[i].number - 1].file_name, dir->d_name);
						user[i].file[user[i].number - 1].type = PRIVATE;
						user[i].file[user[i].number - 1].size = ftell(in1);	
						user[i].no_socket = -1;
					}
				}
				
				
			}

			closedir(d);
 	    }

	}


	// --------------> STABILIREA CONEXIUNII <--------------
	if (argc < 2) 
	{
		fprintf(stderr,"Usage : %s port\n", argv[0]);
		exit(1);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);																		

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	// foloseste adresa IP a masinii
	serv_addr.sin_addr.s_addr = INADDR_ANY;													
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		error("ERROR on binding");

	//sock listen
	listen(sockfd, MAX_CLIENTS); 															
	
	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds); 																
	//setam ca actiune nonblocanta si read-ul, care genereaza semnalul 0 
	FD_SET(0, &read_fds);																	
	fdmax = sockfd;



// --------------> COMUNICAREA <--------------
	do{
START:	tmp_fds = read_fds; 

		number_fd = select(fdmax + 1, &tmp_fds, NULL, NULL, &timeout);
		if(number_fd == -1) 
			error("ERROR in select");
	/*	if(number_fd == 0)
			error("Timeout!!");
	*/
		for(i = 0; i <= fdmax; i++) 
		{
			// the socket data is available to be read
			if (FD_ISSET(i, &tmp_fds)) 														
			{
				//daca apare actiunea de read, i = 0/STDIN_FILENO
				if(i == 0) 
				{
					memset(buffer, 0 , BUFLEN);
		            fgets(buffer, BUFLEN-1, stdin);	
		            //daca citeste "quit" serverul se inchide										
		            if(strncmp(buffer,"quit",3) == 0)
		            {
		                printf("S-a inchis conexiunea cu serverul!!\n");
		                exit(1);     
		            }
				}

				else if (i == sockfd) // a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune; actiunea serverului: accept()
				{
					client_len = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &client_len)) == -1)
						error("ERROR in accept");
					else 
					{
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);										
						if (newsockfd > fdmax)  
							fdmax = newsockfd;

					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(cli_addr.sin_addr), 
																				   ntohs(cli_addr.sin_port), 
																				   newsockfd);	
				}
					
				else // am primit date pe unul din socketii cu care vorbesc cu clientii; actiunea serverului: recv()
				{
					memset(buffer, 0, BUFLEN);
					
					if ((k = recv(i, buffer, sizeof(buffer), 0)) <= 0) 
					{
						if (k == 0) 
						{
							printf("Socket %d hung up\n", i); //conexiunea s-a inchis
						} 
						else 
							error("ERROR in recv");

						close(i); 
						// scoatem din multimea de citire socketul pe care l-am inchis
						FD_CLR(i, &read_fds); 		
					} 
					
					else //recv intoarce > 0
					{ 
						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
				
						// autentificarea
						memset(aux2, 0, BUFLEN);
						strcpy(aux2, buffer);						
						command = strtok(aux2," ");
						
						if(strncmp(command, "login", strlen("login")) == 0)
						{
							command = strtok(NULL, " ");
							user_aux = (char*)malloc(sizeof(char) * (strlen(command)+1));
							strcpy(user_aux, command);
							command = strtok(NULL, " ");
							pass_aux = (char*)malloc(sizeof(char) * (strlen(command)+1));
							strcpy(pass_aux, command);
							
							if(login(user, no_users, i, user_aux, pass_aux) == -1)
							{
								//i se inchide sesiunea clientului din cauza ca sa semnalat brut-force
								close(i); 														
								FD_CLR(i, &read_fds);
							}
							goto START;

						}

						if(strncmp(command, "getuserlist", strlen("getuserlist")) == 0)
						{
							get_user_list(user, no_users, i);
							goto START;
						}

						else if(strncmp(command, "getfilelist", strlen("getfilelist")) == 0)
						{
							command = strtok(NULL," ");
							get_file_list(user, no_users, i, command);
							goto START;
						}

						else if(strncmp(command, "share", strlen("share")) == 0)
						{
							command = strtok(NULL," ");
							share(user, no_users, i, command);
							goto START;	
						}

						else if(strncmp(command, "unshare", strlen("unshare")) == 0)
						{
							command = strtok(NULL," ");
							unshare(user, no_users, i, command);
							goto START;
						}

						else if(strncmp(command, "delete", strlen("delete")) == 0)
						{
							command = strtok(NULL," ");
							delete(user, no_users, i, command);
							goto START;
						}

						else if(strncmp(command, "quit", strlen("quit")) == 0)
						{
							printf("Clientul %d a inchis conexiunea\n", i);
							close(i); 
							FD_CLR(i, &read_fds);
						}

						else if((strncmp(command, "upload", strlen("upload")) == 0) || (files[i].offset > 0))
						{
							
							if(strncmp(command, "upload", strlen("upload")) == 0)
							{
								command = strtok(NULL," ");
								//in file_aux pastram numele fisierului pe care dorim sa-l incarcam pe server 
								file_aux = (char*)malloc(sizeof(char) * (strlen(command) + 1)); 
								memcpy(file_aux, command, strlen(command)+1); 											
							}
							
							files = upload(user, no_users, i, file_aux, files, buffer);
								    
							goto START;		
						}

						else if(strncmp(command, "download", strlen("download")) == 0)
						{
							
							int source, destination;
                            char source_path[200], destination_path[200];

                            memset(source_path, 0 , 200);
                            memset(destination_path, 0, 200);	

							command = strtok(NULL," ");
							user_aux = (char*)malloc(sizeof(char) * (strlen(command)+1));
							//user-ul de la care facem download
							strcpy(user_aux, command);												
							command = strtok(NULL," ");
							file_aux = (char*)malloc(sizeof(char) * (strlen(command)+1));
							//fisierul pe care dorim sa-l downloadam
							strcpy(file_aux, command);												

							download(user, no_users, i, file_aux, user_aux);
							                      			
                           	goto START;
						}

						
						else
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "%s", "Not a valid command!");
							send(i, buffer, strlen(buffer) + 1, 0);
							goto START;
						}


						
					} 
				}
			}
		}
	}FOREVER;
	
	free(user);

	close(sockfd);

	return 0; 
}


