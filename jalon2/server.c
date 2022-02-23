#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common.h"
#include <time.h>
#include <string.h>
#include "msg_struct.h"

#define BACKLOG 20

char users[MSG_LEN];
char is[MSG_LEN];
char time_info[MSG_LEN];

struct Client_info{
	int descripteur;
	char* adresse;
	int port;
	char nickname[NICK_LEN];
	char time[NICK_LEN];
	struct Client_info* next;
};
int socket_listen_and_bind(char *port) {
	int listen_fd = -1;
	if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
	printf("Listen socket descriptor %d\n", listen_fd);

	int yes = 1;
	if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	struct addrinfo indices;
	memset(&indices, 0, sizeof(struct addrinfo));
	indices.ai_family = AF_INET;
	indices.ai_socktype = SOCK_STREAM;
	indices.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	struct addrinfo *res, *tmp;

	int err = 0;
	if (0 != (err = getaddrinfo(NULL, port, &indices, &res))) {
		errx(1, "%s", gai_strerror(err));
	}

	tmp = res;
	while (tmp != NULL) {
		if (tmp->ai_family == AF_INET) {
			struct sockaddr_in *sockptr = (struct sockaddr_in *)(tmp->ai_addr);
			struct in_addr local_address = sockptr->sin_addr;
			printf("Binding to %s on port %hd\n \n",
						 inet_ntoa(local_address),
						 ntohs(sockptr->sin_port));

			if (-1 == bind(listen_fd, tmp->ai_addr, tmp->ai_addrlen)) {
				perror("Binding");
			}
			if (-1 == listen(listen_fd, BACKLOG)) {
				perror("Listen");
			}
			freeaddrinfo(res);
			return listen_fd;
		}
		tmp = tmp->ai_next;
	}
	freeaddrinfo(res);
	return listen_fd;
}
void freeClient(struct Client_info **clients, int fd)
{
	struct Client_info *tmp = *clients;
	struct Client_info *prev;
	if (tmp != NULL && tmp->descripteur == fd) {
			*clients = tmp->next;
			free(tmp);
			return;
	}
	while (tmp != NULL && tmp->descripteur != fd) {
			prev = tmp;
			tmp = tmp->next;
	}
	if (tmp == NULL)
			return;
	prev->next = tmp->next;
	free(tmp);
}
int check_space(char  name[NICK_LEN]){
for (int i = 0; i < strlen(name); i++) {
		if(name[i]==' '){
			return 1;
		}
}
return 0;
}
void Add_Client(struct Client_info** client, int fd, char * addr, int port,char *nick,char* timeinfo)
{
		struct Client_info* new_client= (struct Client_info*)malloc(sizeof(struct Client_info));
 new_client->descripteur = fd;
 new_client->adresse=addr;
 new_client->port=port;
 memset(new_client->nickname,'\0',NICK_LEN);
 strcpy(new_client->nickname,nick);
 memset(new_client->time,'\0',NICK_LEN);
 strcpy(new_client->time,timeinfo);
 new_client->next = (*client);
 (*client) = new_client;
}
int checknick(struct Client_info *c,char * nick){
	struct Client_info *tmp=c;
	while(tmp){
		if(strcmp(tmp->nickname,nick)==0){
			return 0;
		}
		tmp=tmp->next;
	}
	return 1;
}

int nombre_client(struct Client_info *c){
	struct Client_info *tmp=c;
	int n=0;
	while(tmp){
			n++;
		tmp=tmp->next;
	}
	return n;
}
struct Client_info *getuser(struct Client_info *c,char * name){
	struct Client_info *tmp=c;
	struct Client_info *user;
	while(tmp){
			if(strcmp(tmp->nickname,name)==0)
				user=tmp;
		tmp=tmp->next;

	}
return user;
}
void getusers(struct Client_info *c){
	memset(users,'\0',MSG_LEN);
	strcpy(users,"[server] Online users are:\n \t-");
	while(c->next){
		strcat(users,c->nickname);
		strcat(users,"\n \t-");
		c=c->next;
	}
		strcat(users,c->nickname);
		strcat(users,"\n");
}

int ChangeNickName(struct Client_info *c, char * nick,int fd){
		struct Client_info *tmp=c;
		if(!checknick(tmp,nick)){
			return 0;
		}else{
			while(tmp){
					if(tmp->descripteur==fd){
					  memset(tmp->nickname,'\0',NICK_LEN);
					  strcpy(tmp->nickname,nick);
					}
				tmp=tmp->next;
			}
			return 1;
		}
}
void whois(struct Client_info * c){

	//char *is_ptr=is;
	memset(is,'\0',MSG_LEN);
	strcpy(is,c->nickname);
	strcat(is," connected since ");
	strcat(is,c->time);
	strcat(is," with IP address "),
	strcat(is,c->adresse);
	strcat(is, "  and port ");
	char string[20];
	sprintf(string, "%d", c->port);
	strcat(is, string);
	strcat(is,"\n");
}
void get_time_info(){
	time_t rawtime;
	struct tm *info;

//	char time_info[MSG_LEN];
	memset(time_info,'\0',MSG_LEN);
	time(&rawtime);
	/* Get GMT time */
	info = gmtime(&rawtime );
	char ttmp[MSG_LEN];
	memset(ttmp,'\0',MSG_LEN);
	sprintf(ttmp,"%d",(info->tm_year)+1900);
	strcpy(time_info,ttmp);
	strcat(time_info,"/");
	memset(ttmp,'\0',MSG_LEN);
	sprintf(ttmp,"%d",(info->tm_mon)+1);
	strcat(time_info,ttmp);
	strcat(time_info,"/");
	memset(ttmp,'\0',MSG_LEN);
	sprintf(ttmp,"%d",(info->tm_mday));
	strcat(time_info,ttmp);
	strcat(time_info,"@");
	memset(ttmp,'\0',MSG_LEN);
	sprintf(ttmp,"%d",(info->tm_hour+2)%24);
	strcat(time_info,ttmp);
	strcat(time_info,":");
	memset(ttmp,'\0',MSG_LEN);
	sprintf(ttmp,"%d",(info->tm_min));
	strcat(time_info,ttmp);
}
void server(int listen_fd) {

	// Declare array of struct pollfd
	int nfds = 10;
	struct pollfd pollfds[nfds];
	struct Client_info* client_info=NULL;

	int nickname_ok=0;
	int rec1=-1;
	struct message msgstruct;
	// Init first slot with listening socket
	pollfds[0].fd = listen_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	// Init remaining slot to default values
	for (int i = 1; i < nfds; i++) {
		pollfds[i].fd = -1;
		pollfds[i].events = 0;
		pollfds[i].revents = 0;
	}
	// server loop
	while (1) {

		// Block until new activity detected on existing socket
		int n_active = 0;
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll");
		}

		// Iterate on the array of monitored struct pollfd
		for (int i = 0; i < nfds; i++) {

			// If listening socket is active => accept new incoming connection
			if (pollfds[i].fd == listen_fd && pollfds[i].revents & POLLIN) {
				// accept new connection and retrieve new socket file descriptor
				struct sockaddr client_addr;
				socklen_t size = sizeof(client_addr);
				int client_fd;
				if (-1 == (client_fd = accept(listen_fd, &client_addr, &size))) {
					perror("Accept");
				}

				// display client connection information
				struct sockaddr_in *sockptr = (struct sockaddr_in *)(&client_addr);
				struct in_addr client_address = sockptr->sin_addr;
				printf("[Server] Connection succeeded: \n");
				memset(&msgstruct, 0, sizeof(struct message));
				nickname_ok=0;
				while(!nickname_ok){

							if (recv(client_fd, &msgstruct, sizeof(struct message), 0) <= 0) {
								break;
							}
							char nick[msgstruct.pld_len];
							memset(nick,'\0',msgstruct.pld_len);
							if (recv(client_fd, nick, msgstruct.pld_len, 0) <= 0) {
								break;
							}
							nick[msgstruct.pld_len]=0;
							int test_the_nickname=checknick(client_info,nick);
							char prlm[MSG_LEN];
							if(test_the_nickname){
								strcpy(msgstruct.nick_sender,nick);

								printf("--%s--\n",nick );
								get_time_info();
								Add_Client(&client_info,client_fd,inet_ntoa(client_address),ntohs(sockptr->sin_port),nick, time_info );
								sprintf(prlm,"%s [%s]\n","[Server] welcome on the chat",msgstruct.nick_sender);
								nickname_ok=1;
							}else{
								nickname_ok=0;
							 	strcpy(prlm, "[Server] your login is already taken\n");
							}

							msgstruct.type=NICKNAME_NEW;
							msgstruct.pld_len = strlen(prlm)+1;
							strcpy(msgstruct.infos,prlm);
							if (send(client_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
					        break;
					    }
					    if (send(client_fd,prlm, msgstruct.pld_len, 0) <= 0) {
					        break;
					    }

			}
				// store new file descriptor in available slot in the array of struct pollfd set .events to POLLIN
				for (int j = 0; j < nfds; j++) {
					if (pollfds[j].fd == -1) {
						pollfds[j].fd = client_fd;
						pollfds[j].events = POLLIN;
						break;
					}
				}
				// Set .revents of listening socket back to default
				pollfds[i].revents = 0;

			} else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLHUP) { // If a socket previously created to communicate with a client detects a disconnection from the client side
				// display message on terminal
				printf("client on socket %d has disconnected from server\n", pollfds[i].fd);
				// Close socket and set struct pollfd back to default
				close(pollfds[i].fd);
				pollfds[i].events = 0;
				pollfds[i].revents = 0;
			} else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLIN) { // If a socket different from the listening socket is active
				// read data from socket

				memset(&msgstruct, 0, sizeof(struct message));

				rec1=recv(pollfds[i].fd, &msgstruct, sizeof(struct message), 0);
				if (rec1 == -1) {
					perror("Error receiving structure");
					exit(EXIT_FAILURE);
				}
				char buffrec[msgstruct.pld_len+1];
				memset(buffrec, '\0', msgstruct.pld_len+1);
				if(msgstruct.type == ECHO_SEND || msgstruct.type == BROADCAST_SEND||msgstruct.type == UNICAST_SEND){
				if (recv(pollfds[i].fd,buffrec, msgstruct.pld_len, 0) <= 0) {
						break;
				}
			}
				char buffsend[MSG_LEN];
				memset(buffsend, '\0', MSG_LEN);
				printf("Structuce received : pld_len: %i, nick_sender: %s, type: %s, infos: %s\n",msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
				switch (msgstruct.type) {
					case BROADCAST_SEND:
					;int nb=nombre_client(client_info);
					char tmp[MSG_LEN];
					memset(tmp,'\0',MSG_LEN);
					strcpy(tmp,buffrec);
					sprintf(buffrec,"[%s]: %s",msgstruct.nick_sender,tmp);
					msgstruct.pld_len=strlen(buffrec)+1;
					for (int j = 0; j < nb; j++) {
						if(j+4!=pollfds[i].fd){
							printf("%i will recive: %s\n",j+4 ,buffrec);
							if (send(j+4, &msgstruct, sizeof(struct message), 0)<=0) {
								perror("Error receiving structure");
								exit(EXIT_FAILURE);
							}

							if (send(j+4, buffrec, msgstruct.pld_len, 0) <= 0) {
								perror("Sending");
								break;
							}
						}
					}

						;break;
					case NICKNAME_INFOS:
							if(!checknick(client_info,msgstruct.infos)){
								whois(getuser(client_info,msgstruct.infos));
								strcpy(buffsend,is);
								msgstruct.pld_len=strlen(buffsend)+1;
							}else{
								strcpy(buffsend,"this nick doest exits");//name
								msgstruct.pld_len=strlen(buffsend)+1;
								//strcpy(msgstruct.infos,buff);
							};break;
					case NICKNAME_NEW:
					;
							char name[MSG_LEN];
							memset(name,'\0',MSG_LEN);
							strncpy(name,msgstruct.infos,strlen(msgstruct.infos)+1);
							int test_space=check_space(name);
							if(ChangeNickName(client_info,name,pollfds[i].fd)&&!test_space){
								strcpy(msgstruct.nick_sender,name);
								strcpy(buffsend,"[Server] your new name is configurated");
								msgstruct.pld_len=strlen(buffsend)+1;
							}else{
								memset(buffsend,'\0',MSG_LEN);
								if(!test_space)
									strcpy(buffsend,"[Server] your new name is already taken");
								else
									strcpy(buffsend,"[Server] No space please");
								msgstruct.pld_len=strlen(buffsend)+1;
							}break;
					case UNICAST_SEND:
					if(!checknick(client_info,msgstruct.infos)){
						char tmp[MSG_LEN];
						memset(tmp,'\0',MSG_LEN);
						strcpy(tmp,buffrec);
						sprintf(buffrec,"[%s]: %s",msgstruct.nick_sender,tmp);
							msgstruct.pld_len=strlen(buffrec)+1;
						if (send(getuser(client_info,msgstruct.infos)->descripteur, &msgstruct, sizeof(struct message), 0)<=0) {
							perror("Error receiving structure");
							exit(EXIT_FAILURE);
						}


						if (send(getuser(client_info,msgstruct.infos)->descripteur, buffrec, msgstruct.pld_len, 0) <= 0) {
							perror("Sending");
							break;
						}}else{
							strcpy(buffsend,"this nick doest exits");//name
							msgstruct.pld_len=strlen(buffsend)+1;

							if (send(pollfds[i].fd, &msgstruct, sizeof(struct message), 0)<=0) {
								perror("Error receiving structure");
								exit(EXIT_FAILURE);
							}

							if (send(pollfds[i].fd, buffsend, msgstruct.pld_len, 0) <= 0) {
								perror("Sending");
								break;
							}
						}
							break;
					case NICKNAME_LIST:
							getusers(client_info);
							strcpy(buffsend,users);
							msgstruct.pld_len=strlen(buffsend)+1;
							break;
					case ECHO_SEND:
							if(strncmp(buffrec,"/quit",5)){

								printf("message received: %s\n",buffrec);
							}else{
								printf("Client %i closed the connexion\n",pollfds[i].fd );
								freeClient(&client_info,pollfds[i].fd);
								close(pollfds[i].fd);
							 	pollfds[i].fd=-1;
								pollfds[i].events = 0;
								pollfds[i].revents = 0;
								break;
							};
								break;
							default:
							printf("%i\n",msgstruct.type);
							break;
				}
if(pollfds[i].fd>0 && msgstruct.type!=BROADCAST_SEND&& msgstruct.type!=UNICAST_SEND){
				if (send(pollfds[i].fd, &msgstruct, sizeof(struct message), 0)==-1) {
					perror("Error receiving structure");
					exit(EXIT_FAILURE);
				}
				if(msgstruct.type==ECHO_SEND){
				if (send(pollfds[i].fd, buffrec, msgstruct.pld_len, 0) <= 0) {
					perror("Sending");
					break;
				}}else{
					if (send(pollfds[i].fd, buffsend, msgstruct.pld_len, 0) <= 0) {
						perror("Sending");
						break;
					}
				}
				printf("Message sent to Client %i!\n\n",pollfds[i].fd);

				pollfds[i].revents = 0;}
			}
		}
	}
}

int main(int argc, char  *argv[]) {

	// Test argc
	if (argc != 2) {
		printf("Usage: ./server port_number\n");
		exit(EXIT_FAILURE);
	}

	// Create listening socket
	char *port = argv[1];
	int listen_fd = -1;
	if (-1 == (listen_fd = socket_listen_and_bind(port))) {
		printf("Could not create, bind and listen properly\n");
		return 1;
	}
	// Handle new connections and existing ones
	server(listen_fd);

	return 0;
}
