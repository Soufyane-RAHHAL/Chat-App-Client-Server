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
#include <string.h>
#define BACKLOG 20

struct Client_info{
	int descripteur;
	char* adresse;
	int port;
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
void Add_Client(struct Client_info** client, int fd, char * addr, int port)
{
		struct Client_info* new_client= (struct Client_info*)malloc(sizeof(struct Client_info));
 new_client->descripteur = fd;
 new_client->adresse=addr;
 new_client->port=port;
 new_client->next = (*client);
 (*client) = new_client;
}

void server(int listen_fd) {

	// Declare array of struct pollfd
	int nfds = 10;
	struct pollfd pollfds[nfds];
	struct Client_info* client_info=NULL;

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

				printf("Client_fd: %d,  Client_addr: %s, Client_Port: %d\n \n", client_fd,inet_ntoa(client_address),ntohs(sockptr->sin_port));

				//add client information to linked list
				Add_Client(&client_info,client_fd,inet_ntoa(client_address),ntohs(sockptr->sin_port));


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
				char buff[MSG_LEN];
				memset(buff,0,MSG_LEN);

				if (recv(pollfds[i].fd, buff, MSG_LEN, 0) <= 0) {
					perror("Receiving");
					break;
				}
				if(strcmp(buff,"/quit\n")==0){
					printf("Client %i closed the connexion\n",pollfds[i].fd );
					freeClient(&client_info,pollfds[i].fd);
					close(pollfds[i].fd);
					pollfds[i].events = 0;
					pollfds[i].revents = 0;
					break;
				}
				printf("Received,from Client %i : %s", pollfds[i].fd,buff);
				if (send(pollfds[i].fd, buff, strlen(buff), 0) <= 0) {
					perror("Sending");
					break;
				}
				printf("Message sent to Client %i!\n\n",pollfds[i].fd);

				pollfds[i].revents = 0;
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
