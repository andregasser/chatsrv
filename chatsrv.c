#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "log.h"

#define MAX_THREADS 10

/* Function prototypes */
void proc_client(int client_sockfd);

int main()
{
	int i = 0;
	int ret = 0;
	int curr_thread_count = 0;
	pthread_t threads[MAX_THREADS];
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
		ret = pthread_create(&threads[curr_thread_count],
			NULL,
			(void *)&proc_client, 
			(void *)&client_sockfd);
			
		if (ret == 0)
			curr_thread_count++;
	}
	
	for (i = 0; i < curr_thread_count; i++)
	{
		pthread_join(threads[i], NULL);
	}

	return 0;
}

void proc_client(int client_sockfd)
{
	char buffer[1024];

	printf("Foo!\n");

	while (1)
	{
		int len = 0;
		//ioctl(client_sockfd, FIONREAD, &len);
		//if (len > 0) {
	  	//	printf("Has text\n");
	  		len = read(client_sockfd, buffer, 1023);
	  		if (len > 0) {
	  		/* Terminate the data buffer */
	  		buffer[1024] = '\0';
	  		
	  		/* Print out message on console */
	  		//logline(LOG_INFO, "Received: %s", buffer);
	  		printf("Received: %s", buffer);
	  		}
  		//}
	}
}



