#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"
#include "msg_struct.h"

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
	//strcpy(msgstruct.infos,realnick);
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
		}else{
			msgstruct.type = ECHO_SEND;
			if(strncmp(buffsend,"/quit",5)==0){
				online=0;

			}else{
			strcpy(msgstruct.infos,"");
			}
		}
		msgstruct.pld_len = strlen(buffsend);

		// Sending structure

    if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
        break;
    }
		if(msgstruct.type == ECHO_SEND||msgstruct.type == BROADCAST_SEND||msgstruct.type == UNICAST_SEND){
		//	printf("OOKk %s\n", buffsend);
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
		char buffrec[msgstruct.pld_len];
		memset(buffrec,'\0',msgstruct.pld_len);
		if (recv(sockfd, buffrec, msgstruct.pld_len, 0) <= 0) {
			perror("Error receiving structure");
			break;
		}

		printf("Received: %s\n", buffrec);
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
