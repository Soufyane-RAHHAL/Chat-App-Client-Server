#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <err.h>
#include "common.h"
#include "msg_struct.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BACKLOG 20
int etat=1;//pour savoir si on est dans une salon.

int check_space(char  name[NICK_LEN]){
for (int i = 0; i <= strlen(name); i++) {
		if(name[i]==' '){
			return 1;
		}
}
return 0;
}
void echo_client(int sockfd) {
	struct message msgstruct;
	struct pollfd poll_fds_C[2];
	//char *buff;
	char nickname[MSG_LEN];
	char buffsend[MSG_LEN];
	int nickname_ok=0;
	int online=1;
	int n;

	poll_fds_C[0].fd=STDIN_FILENO;
	poll_fds_C[0].events=POLLIN;
	poll_fds_C[0].revents=0;

	poll_fds_C[1].fd=sockfd;
	poll_fds_C[1].events=POLLIN;
	poll_fds_C[1].revents=0;
	memset(&msgstruct, 0, sizeof(struct message));
	memset(nickname,'\0',MSG_LEN);
		memset(buffsend,'\0',MSG_LEN);

	while(!nickname_ok){
		n=0;
	printf("[Server] : please login with /nick <your pseudo>\n");
	memset(nickname,'\0',MSG_LEN);
	while((nickname[n++]=getchar())!='\n'){}
	nickname[n-1]=0;
	while (strncmp(nickname,"/nick ",6)){
		n=0;
		printf("[Server] please login with /nick <your pseudo>\n");
		memset(nickname,'\0',MSG_LEN);
		while((nickname[n++]=getchar())!='\n'){}
		nickname[n-1]=0;
	}
	strcpy(nickname,nickname+6);
	int space=check_space(nickname);
	while(space){
		n=0;
		printf("No spaces please\n" );
		printf("[Server] please login with /nick <your pseudo>\n");
		memset(nickname,'\0',MSG_LEN);
		while((nickname[n++]=getchar())!='\n'){}
		nickname[n-1]=0;
		strcpy(nickname,nickname+6);
		space=check_space(nickname);
	}
	strcpy(msgstruct.infos,nickname);
	msgstruct.type=NICKNAME_NEW;
	msgstruct.pld_len = n-1;
	if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			perror("Error");
	}
	if (send(sockfd,nickname, msgstruct.pld_len, 0) <= 0) {
				perror("Error");
	}
	if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
		perror("Error");
	}
	char buffrec[msgstruct.pld_len];
	memset(buffrec,'\0',msgstruct.pld_len);
	// Receiving message
	if (recv(sockfd, buffrec, msgstruct.pld_len, 0) <= 0) {
	perror("Error");
	}
	printf("%s",buffrec);
	if(strcmp(buffrec,"[Server] your login is already taken\n")==0){
		nickname_ok=0;
	}else{
			nickname_ok=1;
		}
}
	while (1) {
		if(poll(poll_fds_C,2,-1)<=0){
			perror("POLL");
			break;
		}
			if(poll_fds_C[0].revents&POLLIN){
		// Cleaning memory
		memset(buffsend, '\0', MSG_LEN);
		memset(&msgstruct, 0, sizeof(struct message));

		strcpy(msgstruct.nick_sender, nickname);
    strncpy(msgstruct.infos, "\0", 1);
		// Getting message from client
		if(online){
		n = 0;
		while ((buffsend[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
	}// Sending message (ECHO)
		buffsend[n-1]=0;
		//get The message Type
		if (strncmp(buffsend,"/whois ",7)==0){
				msgstruct.type = NICKNAME_INFOS;
				strcpy(buffsend,buffsend+7);
				strcpy(msgstruct.infos,buffsend);
		}else if(strncmp(buffsend,"/who",4)==0){
			msgstruct.type = NICKNAME_LIST;
		}else if(strncmp(buffsend,"/msgall ",8)==0){
			msgstruct.type = BROADCAST_SEND;
			strcpy(buffsend,buffsend+8);
		}else if(strncmp(buffsend,"/msg ",5)==0){
			msgstruct.type = UNICAST_SEND;
			strcpy(buffsend,buffsend+5);
			char delim[]=" ";
			char *ptr =strtok(buffsend,delim);
			strcpy(msgstruct.infos,ptr);
			char private_msg[MSG_LEN];
			memset(private_msg,'\0',MSG_LEN);
			ptr=strtok(NULL,delim);
			while(ptr!=NULL){
				strcat(private_msg,ptr);
				strcat(private_msg," ");
				ptr=strtok(NULL,delim);
			}
			memset(buffsend,'\0',MSG_LEN);
			strcpy(buffsend,private_msg);
		}else if(strncmp(buffsend,"/nick ",6)==0){
			msgstruct.type = NICKNAME_NEW;
			strcpy(buffsend,buffsend+6);
			strcpy(msgstruct.infos,buffsend);
		}else if(strncmp(buffsend,"/create ",8)==0){
			msgstruct.type = MULTICAST_CREATE;
			strcpy(buffsend,buffsend+8);
			strcpy(msgstruct.infos,buffsend);

		}
		else if(strncmp(buffsend,"/channel_list",13)==0){
			msgstruct.type = MULTICAST_LIST;
		}
		else if(strncmp(buffsend,"/join ",6)==0){
			msgstruct.type = MULTICAST_JOIN;
			strcpy(buffsend,buffsend+6);
			strcpy(msgstruct.infos,buffsend);
		}
		else if(strncmp(buffsend,"/send ",6)==0){
			// if (send(5,nickname, msgstruct.pld_len, 0) <= 0) {
			// 			perror("Error");
			// }
			msgstruct.type = FILE_REQUEST;
			 strcpy(buffsend,buffsend+6);
			 char delim[]=" ";
			 char *ptr =strtok(buffsend,delim);
			 strcpy(msgstruct.infos,ptr);
			 char file_name[MSG_LEN];
			 memset(file_name,'\0',MSG_LEN);
			 ptr=strtok(NULL,delim);
			 while(ptr!=NULL){
			 	strcat(file_name,ptr);
			 	strcat(file_name," ");
			 	ptr=strtok(NULL,delim);
			 }
			 memset(buffsend,'\0',MSG_LEN);
			 strcpy(buffsend,file_name);
		}else{
			msgstruct.type = ECHO_SEND;

			if(strncmp(buffsend,"/quit",5)==0){
				if(etat==1)
						online=0;
				else {
						msgstruct.type = MULTICAST_QUIT;
						strcpy(buffsend,buffsend+6);
					 	strcpy(msgstruct.infos,buffsend);
						etat=1;
				}
			}else{
			strcpy(msgstruct.infos,"");
			}
		}
		msgstruct.pld_len = strlen(buffsend);

		// Sending structure
	//if(msgstruct.type!=FILE_ACCEPT){
    if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
        break;
    }
	//}
		if(msgstruct.type == ECHO_SEND||msgstruct.type == BROADCAST_SEND||msgstruct.type == UNICAST_SEND||msgstruct.type==FILE_REQUEST){
	  if (send(sockfd,buffsend, msgstruct.pld_len, 0) <= 0) {
        break;
    }


	}
		printf("Message sent!\n");
		poll_fds_C[0].revents=0;
	}else if(poll_fds_C[1].revents &POLLIN) {
		// Cleaning memory
if(!online){
		memset(&msgstruct, 0, sizeof(struct message));
		// Receiving message

			printf("You have disconnected\n" );
			break;
		}else{
		if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
			perror("Error receiving structure");
			break;
		}

		//strcpy(nickname,msgstruct.nick_sender);
		char buffrec[msgstruct.pld_len];
		memset(buffrec,'\0',msgstruct.pld_len);
		if (recv(sockfd, buffrec, msgstruct.pld_len, 0) <= 0) {
			perror("Error receiving structure");
			break;
		}

		if(msgstruct.type==FILE_ACCEPT){
			//strcpy(buffrec,buffrec);
			struct sockaddr_in sock_host;
			 int sock;
			 char delim2[]=":";
			 char *ptr2 =strtok(buffrec,delim2);
			char add[MSG_LEN]; memset(add,'\0',MSG_LEN);
			strcpy(add,ptr2);
			char port_name[MSG_LEN];
			memset(port_name,'\0',MSG_LEN);
			ptr2=strtok(NULL,delim2);
			while(ptr2!=NULL){
				strcat(port_name,ptr2);
				strcat(port_name,":");
				ptr2=strtok(NULL,delim2);
			}
			char delim3[]="-";
			char *ptr3 =strtok(port_name,delim3);
		 char port[MSG_LEN]; memset(port,'\0',MSG_LEN);
		 strcpy(port,ptr3);
		 char name[MSG_LEN];
		 memset(name,'\0',MSG_LEN);
		 ptr3=strtok(NULL,delim3);
		 while(ptr3!=NULL){
			 strcat(name,ptr3);
			 strcat(name,"-");
			 ptr3=strtok(NULL,delim3);
		 }

			sock=socket(AF_INET, SOCK_STREAM,0);
			memset(&sock_host,'\0',sizeof(sock_host));
			sock_host.sin_family=AF_INET;
			sock_host.sin_port=htons(atoi(port)+1);
			inet_aton(add,&sock_host.sin_addr);
			connect(sock,(struct sockaddr *)&sock_host,sizeof(sock_host));
name[strlen(name)-3]=0;
			printf("-%s-\n",name );

			FILE *file=fopen(name,"rb");

			fseek(file, 0, SEEK_END);
			int file_size = ftell(file);
			fseek(file, 0, SEEK_SET);  /* same as rewind(f); */

			char *string = malloc(file_size + 1);
			fread(string, 1, file_size, file);
			fclose(file);

			string[file_size] = 0;
			if (send(sock, &file_size, sizeof(int), 0) <= 0) {
					break;
			}
			if (send(sock, string, file_size, 0) <= 0) {
					break;
			}
			free(string);
			close(sock);
		}

		if(msgstruct.type == NICKNAME_NEW){
			if(strcmp(buffrec,"your new name is configurated")==0){
					strcpy(nickname,msgstruct.infos);
					strcpy(msgstruct.nick_sender,msgstruct.infos);
			}
		}
		if(msgstruct.type==MULTICAST_JOIN||msgstruct.type==MULTICAST_CREATE){

			if(strncmp(buffrec,"You have joined channel[",24)==0){
				etat=2;
			}
		}

if(msgstruct.type==FILE_REQUEST){
	int file_request=1;
	 char delim[]="@";
	 //printf("%s\n",buffrec );
	char *ptr =strtok(buffrec,delim);
	strcpy(buffrec,ptr);
	char adresse_port[MSG_LEN];
	memset(adresse_port,'\0',MSG_LEN);
	ptr=strtok(NULL,delim);
	while(ptr!=NULL){
		strcat(adresse_port,ptr);
		strcat(adresse_port,"@");
		ptr=strtok(NULL,delim);
	}
	char delim2[]=":";
	char *ptr2 =strtok(adresse_port,delim2);
	char add[MSG_LEN]; memset(add,'\0',MSG_LEN);
	strcpy(add,ptr2);
	char port[MSG_LEN];
	memset(port,'\0',MSG_LEN);
	ptr2=strtok(NULL,delim2);
	while(ptr2!=NULL){
		strcat(port,ptr2);
		strcat(port,":");
		ptr2=strtok(NULL,delim2);
	}
	port[strlen(port)-2]=0;
	//printf("%s:%s\n",add,port );
	while(file_request){
		printf("%s\n",buffrec );
		n=0;
		char answer[MSG_LEN];
		memset(answer,'\0',MSG_LEN);
		while((answer[n++]=getchar())!='\n'){}
		answer[n-1]=0;


		if(strcmp(answer,"Y")==0){
			msgstruct.type=FILE_ACCEPT;
			strcpy(msgstruct.infos,msgstruct.nick_sender);
			strcpy(msgstruct.nick_sender, nickname);
			memset(buffsend,'\0',MSG_LEN);
			strcpy(buffsend,add);
			strcat(buffsend,":");
			strcat(buffsend,port);
			strcpy(buffsend,buffsend+1);
			msgstruct.pld_len=strlen(buffsend)+1;
			if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
					break;
			}
			 if (send(sockfd,buffsend, msgstruct.pld_len, 0) <= 0) {
			 		break;
		 }
		 struct sockaddr_in saddr_in;
			int sock;
			sock =socket(PF_INET,SOCK_STREAM,0);
			if(sock == -1){
				perror("socket");exit(EXIT_FAILURE);
			}
			memset(&saddr_in,0,sizeof(saddr_in));
			saddr_in.sin_family=AF_INET;
			saddr_in.sin_port=htons(atoi(port)+1);
			saddr_in.sin_addr.s_addr=INADDR_ANY;
			if(bind(sock,(struct sockaddr *)&saddr_in,sizeof(saddr_in))==-1){
					perror("bind");
					exit(EXIT_FAILURE);
			}
			if(listen(sock,SOMAXCONN)==-1){
				perror("listen");exit(EXIT_FAILURE);
			}
			struct sockaddr client_addr;
			socklen_t size = sizeof(client_addr);
			int client_fd;
			if (-1 == (client_fd = accept(sock, &client_addr, &size))) {
				perror("Accept");
			}
			int siz=0;
			if (recv(client_fd, &siz, sizeof(int), 0) <= 0) {
					break;
			}
			char file_c[siz];memset(file_c,'\0',siz);
			if (recv(client_fd, file_c, siz, 0) <= 0) {
					break;
			}
			strcpy(port,port+6);
			port[strlen(port)-5]=0;
			strcat(port,"-recived.txt");
				FILE *fp;
							fp=fopen(port,"a");
							fputs(file_c,fp);
							fclose(fp);
			printf("the file saved in %s\n",port );
			close(client_fd);
			close(sock);
			file_request=0;

		}else if(strcmp(answer,"N")==0){
			msgstruct.type=FILE_REJECT;
			msgstruct.pld_len=1;
			file_request=0;
		}

	}
	strcpy(msgstruct.infos,msgstruct.nick_sender);
	strcpy(msgstruct.nick_sender, nickname);
	if(msgstruct.type!=FILE_ACCEPT){
	if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
	}
}

	printf("Message sent!\n");

}else{
		printf("Received: %s \n", buffrec);}
		poll_fds_C[0].revents=0;
	}
	}
	}
}

int handle_connect(char *hostname, char *port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(hostname, port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			perror("Socket");

			exit(EXIT_FAILURE);
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: ./client hostname port_number\n");
		exit(EXIT_FAILURE);
	}
	char *hostname = argv[1];
	char *port = argv[2];
	int sfd;
	sfd = handle_connect(hostname,port);
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}
