#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "log.h"

int main()
{
	int server_sockfd, client_sockfd;
	int server_len, client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	
	set_loglevel(LOG_DEBUG);

	logline(LOG_INFO, "-----------");
	logline(LOG_INFO, "Chat Server");
	logline(LOG_INFO, "-----------");

	/* Create socket */
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Name the socket */
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(5555);
	server_len = sizeof(server_address);
	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);

	/* Create a connection queue and wait for incoming connections */
	listen(server_sockfd, 5);
	while (1)
	{
		char ch;
		logline(LOG_DEBUG, "Waiting for incoming connections...");
		
		/* Accept a client connection */
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);

		/* A connection between a client and the server has been established.
		 * Now create a new thread and handover the client_sockfd
		 */
		


		/* Read and write to client */
		read(client_sockfd, &ch, 1);
		ch++;
		write(client_sockfd, &ch, 1);
		close(client_sockfd);
	}

	return 0;
}
