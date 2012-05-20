#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include "log.h"
#include "llist.h"


#define MAX_THREADS 10


/* Global vars */
struct sockaddr_in server_address;
struct sockaddr_in client_address;
int server_sockfd, client_sockfd;
int server_len, client_len;
llist_t ll_clients;


/* Function prototypes */
void proc_client(int *arg);
void shutdown_server(int sig);


/*
 * Main program
 */
int main()
{
	int i = 0;
	int ret = 0;
	int optval;
	int curr_thread_count = 0;
	pthread_t threads[MAX_THREADS];
			
	/* Setup signal handler */
	(void) signal(SIGINT, shutdown_server);
	
	/* Setup linked list */
	llist_init(&ll_clients);
	
	/* Setup logging */
	set_loglevel(LOG_DEBUG);

	logline(LOG_INFO, "-----------");
	logline(LOG_INFO, "Chat Server");
	logline(LOG_INFO, "-----------");

	/* Create socket */
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	optval = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0)
	{
		perror(strerror(errno));
		exit(-4);
	}

	/* Name the socket */
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(5555);
	server_len = sizeof(server_address);
	if (bind(server_sockfd, (struct sockaddr *)&server_address, server_len) != 0)
	{
		perror(strerror(errno));
		exit(-1);
	}

	/* Create a connection queue and wait for incoming connections */
	if (listen(server_sockfd, 5) != 0)
	{
		perror(strerror(errno));
		exit(-2);
	}
	
	while (1)
	{
		logline(LOG_INFO, "Waiting for incoming connection...");
		
		/* Accept a client connection */
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);

		if (client_sockfd > 0)
		{
			/* A connection between a client and the server has been established.
			 * Now create a new thread and handover the client_sockfd
			 */
			if (curr_thread_count < 10)
			{
				ret = pthread_create(&threads[curr_thread_count],
					NULL,
					(void *)&proc_client, 
					(void *)&client_sockfd);
			
				if (ret == 0)
				{
					curr_thread_count++;
					llist_insert_data(0, &client_sockfd, &ll_clients);
				}
				else
				{
					close(client_sockfd);
				}
			}
			else
			{
				logline(LOG_INFO, "Max. connections reached. Connection dropped.");
				close(client_sockfd);
			}
		}
		else
		{
			/* Connection could not be established. Post error and exit. */
			perror(strerror(errno));
			exit(-3);
		}
	}
	
	for (i = 0; i < curr_thread_count; i++)
	{
		pthread_join(threads[i], NULL);
	}

	return 0;
}


/*
 * This method is run on a per-connection basis in a separate thred.
 */
void proc_client(int *arg)
{
	char buffer[1024];
	int ret;
	int len;
	int socksize;
	int client_sockfd;
	fd_set readfds;
	
	client_sockfd = (int)(*arg);
	
	FD_ZERO(&readfds);
	FD_SET(client_sockfd, &readfds);

	socksize = sizeof(client_address);

	logline(LOG_INFO, "Server is listening on socket id %d...", client_sockfd);

	while (1)
	{
		ret = select(FD_SETSIZE, &readfds, (fd_set *)0, 
			(fd_set *)0, (struct timeval *) 0);
		if (ret < 1)
		{
			perror("Error in thread");
		}
		else
		{
			ioctl(client_sockfd, FIONREAD, &len);
			if (len > 0)
			{
				/* Read data from stream */
				len = recvfrom(client_sockfd, buffer, sizeof(buffer), 0, 
					(struct sockaddr *)&client_address, &socksize);
		  		buffer[len] = '\0';
		  		
		  		logline(LOG_INFO, "Received: %s", buffer);								
			}
		}
	}
}


/*
 * Shuts down the server properly by freeing all allocated resources.
 */
void shutdown_server(int sig)
{
	int i;
	int *client_sockfd;

	if (sig == SIGINT)
	{
		logline(LOG_INFO, "Server shutdown requested per SIGINT. Performing cleanup ops now.");
		
		/* Close all socket connections immediately */
		logline(LOG_INFO, "Closing socket connections...");		
		for (i = 0; i < llist_get_count(&ll_clients); i++)
		{
			llist_find_data(i, (void *)&client_sockfd, &ll_clients);
			close(*client_sockfd);
		}
		
		/* TODO: Cleanup linked list items (remove) */

		/* Close listener connection */
		logline(LOG_INFO, "Shutting down listener...");
		close(server_sockfd);

		/* Exit process */		
		logline(LOG_INFO, "Exiting. Byebye.");
		exit(0);
	}
}

