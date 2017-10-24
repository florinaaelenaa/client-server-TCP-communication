#include "utilities.h"

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    //timeout-ul setat pentru select()
    struct timeval timeout;                                                             
    timeout.tv_sec = 4;
    timeout.tv_usec = 0;

    //multimea de citire folosita in select()
    fd_set read_fds;                                                                    
    fd_set tmp_fds;                                                                     
   
    //valoare maxima file descriptor din multimea read_fds
    int fdmax;                                                                          
    int i, number_fd, j, ok;
    char *command;
    char buffer[BUFLEN], aux[BUFLEN], aux2[BUFLEN], user_aux[BUFLEN], file_aux[BUFLEN];

    DIR *d;
    struct dirent *dir;

    FILE *l;
    l = fopen("log", "wt");
    fseek(l, 0, SEEK_SET);

    TRANSFER *files;
    files = (TRANSFER*)malloc(sizeof(TRANSFER) * (MAX_CLIENTS + 1));
    for(i = 0; i < MAX_CLIENTS + 1; ++i)
    {   
        files[i].file_descriptor = 0;
        files[i].offset = 0;
    }

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);                                                                 
    FD_ZERO(&tmp_fds);

    if (argc < 3) 
    {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    //Ii trmit portul la care trebuie sa se conecteze ca fiind al doilea argument
    serv_addr.sin_port = htons(atoi(argv[2]));                                          
    //Ii trimit IP-ul la care trebuie sa se conecteze ca fiind primul argument
    inet_aton(argv[1], &serv_addr.sin_addr);                                            
        
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd, &read_fds);                                                          
    //adaugam STDIN_FILENO(pt citire de la tastatura) in multimea read_fds
    FD_SET(0, &read_fds);      
    fdmax = sockfd;

    do{
        tmp_fds = read_fds;
        
        number_fd = select(fdmax + 1, &tmp_fds, NULL, NULL, &timeout);
        if (number_fd == -1) 
            error("ERROR in select");
     /*   if(number_fd == 0)
            error("Timeout!!");
    */
        for( i = 0; i <= sockfd; i++ )
            // the socket data is available to be read
            if( FD_ISSET(i, &tmp_fds) )                                                 
            {
                // input is readable
                if( i == 0)                                                             
                {
                    memset(buffer, 0 , BUFLEN);
                    memset(aux, 0 , BUFLEN);
                    fgets(buffer, BUFLEN-1, stdin);
                    strcpy(aux, buffer);
                    command = strtok(aux," ");
                    
                    
                    if(strcmp(command, "login") == 0)
                    {
                        command = strtok(NULL, " ");
                        strcpy(user_aux, command);
                        //trimit cererea serverului
                        n = send(sockfd,buffer,strlen(buffer)+1, 0);                     
                        if (n < 0) 
                             error("ERROR writing to socket");
                    } 

                    else
                        printf("%d Utilizator neautentificat!\n", UNLOGGED_USER);
                    
                    
                }

                //socket is readable
                else                                                                        
                {
                    memset(buffer, 0 , BUFLEN);
                    n = recv(i, buffer, BUFLEN, 0);
                    if( n > 0)
                    {
                        if( atoi(buffer) == WRONG_PASS)
                            {
                                printf("%d Wrong user/password\n", WRONG_PASS);
                                fprintf(l,"%d Wrong user/password\n", WRONG_PASS);

                            }
                        if(atoi(buffer) == BRUTE_FORCE)
                            {
                                printf("%d Brute force!\n", BRUTE_FORCE);
                                fprintf(l, "%d Brute force!\n", BRUTE_FORCE);
                            }

                        if( (buffer[0] - 48) == LOGGED_USER)
                        {

                         do{
                        START:  fprintf(stdout,"%s> ", user_aux);
                                fprintf(l, "%s> ", user_aux);
                                fflush(stdout);
                                
                                memset(buffer, 0 , BUFLEN);
                                memset(aux2, 0 , BUFLEN);
                                
                                fgets(buffer, BUFLEN-1, stdin);
                                strcpy(aux2, buffer);

 //--------------------------------------------------> UPLOAD <----------------------------------------------------    
                                
                                if(strncmp(buffer, "upload", strlen("upload")) == 0)
                                {
                                    char source_path[BUFLEN];
                                    int source, k;

                                    command = strtok(buffer, " ");
                                    command = strtok(NULL," ");
                                    memset(file_aux, 0, BUFLEN);
                                    strcpy(file_aux, command);

                                    memset(source_path, 0,  100);
                                    memset(aux, 0,  BUFLEN);
                                    if (getcwd(aux, sizeof(aux)) == NULL)  //directorul in care il cautam
                                        perror("getcwd() error");
                                    
                                    ok = 0;
                                    d = opendir(aux);                                                           
                                    if(d)
                                        while ((dir = readdir(d)) != NULL)
                                            if( strncmp(dir->d_name, file_aux, strlen(dir->d_name)) == 0 ) 
                                            {
                                                ok = 1;
                                                break;
                                            }

                                    //nu a gasit fisierul pe care sa-l uploadeze        
                                    if(ok == 0)                                                 
                                    {
                                        printf("%d Fisisierul nu exista!\n", UNEXISTING_FILE);
                                        fprintf(l, "%d Fisisierul nu exista!\n", UNEXISTING_FILE);
                                        goto START;
                                    }
                                    else
                                        {
                                            strcpy(buffer, aux2);
                                            //trimite serverului bufferul initial ca sa stie ca e vorba de un upload
                                            send(sockfd, aux2, strlen(aux2)+1, 0);  
                                            //daca primeste mesaj imediat dupa send inseamna ca fisierul deja exista pe server            
                                            if ( FD_ISSET(i, &tmp_fds) )                        
                                            { 
                                                memset(buffer, 0, BUFLEN);
                                                recv(i, buffer, BUFLEN, 0);
                                                if(atoi(buffer) == EXISTING_FILE)
                                                    {
                                                        printf("%d Fisier deja incarcat pe server!\n", EXISTING_FILE);
                                                        fprintf(l, "%d Fisier deja incarcat pe server!\n", EXISTING_FILE);
                                                        goto START;
                                                    }

                                            }
                                            strcat(aux,"/");
                                            strcat(source_path, aux);
                                            strncat(source_path, file_aux, strlen(file_aux)-1);    //sa nu ia si \0 in componenta numelui                             
                                            
                                            //printf("%s\n", source_path);

                                            source = open(source_path, O_RDONLY);
                                            if(source == -1)
                                                printf("nu se deschide sursa\n");
                                            lseek(source, 0, SEEK_SET);

                                            do
                                            {
                                                memset(buffer, 0, BUFLEN);
                                                k = read(source, buffer, 4096);
                                              
                                                if (k == -1) 
                                                {
                                                    printf("Error reading file.\n");
                                                    exit(1);
                                                }

                                                if (k == 0) 
                                                    break;  
                                                
                                                n = send(i, buffer, k, 0);
                                                if (n < 0) 
                                                  error("ERROR writing to socket");


                                            }FOREVER;
                                            memset(buffer, 0 , BUFLEN);
                                            recv(i, buffer, BUFLEN, 0);
                                            printf("%s\n", buffer);
                                            //fprintf(l, "%s\n",buffer );
                                            goto START;

                                        }                                    
                               
                                }
                                
                                n = send(sockfd, buffer, strlen(buffer)+1, 0);
                                
                                if (n < 0) 
                                    error("ERROR writing to socket");  
     
                                if(strncmp(buffer, "login", strlen("login")) == 0)
                                    {
                                        printf("%d Sesiune deja deschisa!\n", SESSION_ALREADY_OPEN );
                                        fprintf(l, "%d Sesiune deja deschisa!\n", SESSION_ALREADY_OPEN);
                                    }
                                
                                else if(strncmp(buffer, "logout", strlen("logout")) == 0)
                                    break; 
                                                                 
                                else
                                {
                                    memset(buffer, 0 , BUFLEN);
                                    n = recv(i, buffer, BUFLEN, 0);
                                    if( n > 0)
                                    {
                                        if( atoi(buffer) == UNEXISTING_USER)
                                            {
                                                printf("%d Ultilizator inexistent!\n", UNEXISTING_USER);
                                                fprintf(l, "%d Ultilizator inexistent!\n", UNEXISTING_USER);
                                            }
                                        else if(atoi(buffer) == FILE_ALREADY_SHARED)
                                            {
                                                printf("%d Fisier deja partajat!\n", FILE_ALREADY_SHARED);
                                                fprintf(l, "%d Fisier deja partajat!\n", FILE_ALREADY_SHARED);
                                            }
                                        else if(atoi(buffer) == FILE_ALREADY_PRIVATE)
                                            {
                                                printf("%d Fisier deja privat!\n", FILE_ALREADY_PRIVATE);
                                                fprintf(l, "%d Fisier deja privat!\n", FILE_ALREADY_PRIVATE);
                                            }
                                        else if(atoi(buffer) == UNEXISTING_FILE)
                                            {
                                                printf("%d Fisier inexistent pe server!\n", UNEXISTING_FILE); 
                                                fprintf(l, "%d Fisier inexistent pe server!\n", UNEXISTING_FILE);
                                            }
                                        else if(atoi(buffer) == EXISTING_FILE)
                                            {
                                                printf("%d Fisier deja incarcat pe server aici!\n", EXISTING_FILE);
                                                fprintf(l, "%d Fisier deja incarcat pe server aici!\n", EXISTING_FILE);
                                            }
                                        else if(atoi(buffer) == FORBIDDEN_DOWNLOAD)
                                            {
                                                printf("%d Fisier privat, nu permite download!\n", FORBIDDEN_DOWNLOAD);
                                                fprintf(l,"%d Fisier privat, nu permite download!\n", FORBIDDEN_DOWNLOAD);
                                            }
                                        else 
                                        {
                                            fprintf(stdout, "%s\n", buffer); 
                                            fprintf(l, "%s\n", buffer);
                                            fflush(stdout);
                                        }
                                    }
                                    if( n == 0 ) 
                                    {
                                        printf("S-a inchis conexiunea cu serverul!!\n");
                                        exit(1);
                                    }   

                                }

                            }FOREVER;
                        }
                    }
                        
                    if( n == 0 )                                                   
                    {
                        printf("S-a inchis conexiunea cu serverul!!\n");
                        exit(1);
                    }
                }
            }
            
            

    }FOREVER;

    close(sockfd);
    fclose(l);

    return 0;
}


