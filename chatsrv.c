/*
 * Chat Server
 *
 * Features
 * - Fast reuse of sockets in case of TIME_WAIT
 * - Graceful server shutdown using SIGINT
 * - Multi-Threading using reader/writer lock on linked list
 * - Uses logging code (data/time) and log levels
 *
 */

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
#include <regex.h>
#include <getopt.h>
#include <limits.h>
#include "log.h"
#include "llist.h"


/* Define some constants */
#define APP_NAME        "CHATSRV" /* Name of applicaton */
#define APP_VERSION     "0.1"     /* Version of application */
#define MAX_THREADS     10


/* Typedefs */
typedef struct 
{
	int client_sockfd;
	char nickname[20];
	struct sockaddr_in client_address;
} client_info;

typedef struct 
{
	char *ip;
	int port;
	int help;
	int loglevel;
	int version;
} cmd_params;


/* Global vars */
struct sockaddr_in server_address;
int server_sockfd;
int server_len, client_len;
llist_t ll_clients;
cmd_params *params;


/* Function prototypes */
int parse_cmd_args(int *argc, char *argv[]);
void proc_client(client_info *arg);
void process_msg(char *message, int self_sockfd);
void send_broadcast_msg(char *message, int self_sockfd);
void chomp(char *s);
void change_nickname(char *oldnickname, char *newnickname);
void shutdown_server(int sig);
int get_client_info_idx_by_sockfd(int sockfd);
int get_client_info_idx_by_nickname(char *nickname);
void display_help_page(void);
void display_version_info(void);
void show_gnu_banner(void);


/*
 * Main program
 */
int main(int argc, char *argv[])
{
	int i = 0;
	int ret = 0;
	int optval;
	int curr_thread_count = 0;
	struct sockaddr_in client_address;
	int client_sockfd;
	pthread_t threads[MAX_THREADS];
			
	/* Parse commandline args */
	params = malloc(sizeof(cmd_params));
	ret = parse_cmd_args(&argc, argv);
	if (params->help)
	{
		display_help_page();
		return 0;
	}
	if (params->version)
	{
		display_version_info();
		return 0;
	}
	if (ret < 0)
	{
		if (ret == -2)
			logline(LOG_ERROR, "Error: Invalid port range specified (-p)");
		if (ret == -6)
			logline(LOG_ERROR, "Error: Invalid log level option specified (-l).");
		logline(LOG_ERROR, "Use the -h option if you need help.");
		return ret;
	}
	

	/* Set log level */
	switch (params->loglevel)
	{
		case 1: set_loglevel(LOG_ERROR); break;
		case 2: set_loglevel(LOG_INFO); break;
		case 3: set_loglevel(LOG_DEBUG); break;
		default: set_loglevel(LOG_ERROR);
	}

	show_gnu_banner();
			
	/* Setup signal handler */
	signal(SIGINT, shutdown_server);
	signal(SIGTERM, shutdown_server);
	
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
	server_address.sin_addr.s_addr = inet_addr(params->ip);
	server_address.sin_port = htons(params->port);
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
	
	logline(LOG_INFO, "Server listening on %s, port %d", params->ip, params->port);
	
	while (1)
	{
		logline(LOG_INFO, "Waiting for incoming connection...");
		
		/* Accept a client connection */
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);

		if (client_sockfd > 0)
		{
			logline(LOG_INFO, "Server accepted new connection on socket id %d", client_sockfd);
			
			/* A connection between a client and the server has been established.
			 * Now create a new thread and handover the client_sockfd
			 */
			if (curr_thread_count < 10)
			{
				/* Prepare client infos in handy structure */
				client_info *ci = (client_info *)malloc(sizeof(client_info));
				ci->client_sockfd = client_sockfd;
				ci->client_address = client_address;
				
				sprintf(ci->nickname, "anonymous_%d", client_sockfd);
				
				/* Pass client info and invoke new thread */	
				ret = pthread_create(&threads[curr_thread_count],
					NULL,
					(void *)&proc_client, 
					(void *)ci);
			
				if (ret == 0)
				{
					pthread_detach(threads[curr_thread_count]);
					
					//send_broadcast_msg("New user joined\n", 0);
					
					curr_thread_count++;
					
					/* Add client info to linked list */
					llist_insert_data(llist_get_count(&ll_clients), ci, &ll_clients);
				}
				else
				{
					free(ci);
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
	
	//for (i = 0; i < curr_thread_count; i++)
	//{
	//	pthread_join(threads[i], NULL);
	//}

	free(params);

	return 0;
}


/*
 * Parse command line arguments and store them in a global struct.
 */
int parse_cmd_args(int *argc, char *argv[])
{
	int option_index = 0;
	int c;

	/* Init struct */
	params->ip = "127.0.0.1";
	params->port = 5555;
	params->help = 0;
	params->loglevel = LOG_INFO;
	params->version = 0;

	static struct option long_options[] = 
	{
		{ "ip",			required_argument, 0, 'i' },
		{ "port",		required_argument, 0, 'p' },
		{ "help",		no_argument,       0, 'h' },
		{ "version",	no_argument,       0, 'v' },
		{ "loglevel",	required_argument, 0, 'l' },
		{ 0, 0, 0, 0 }
	};

	while (1)
	{
		c = getopt_long(*argc, argv, "i:p:hvl:", long_options, &option_index);

		/* Detect the end of the options */
		if (c == -1)
			break;

		switch (c)
		{
			case 'i':
				params->ip = optarg;
				break;
			case 'p':
				params->port = atoi(optarg);
				if ((params->port < 1) || (params->port > 65535))
					return -2;
				break;
			case 'h':
				params->help = 1;
				break;
			case 'v':
				params->version = 1;
				break;
			case 'l':
				params->loglevel = atoi(optarg);
	            if ((params->loglevel < 1) || (params->loglevel > 3))
	            	return -6;
				break;
		}
	}

	return 0;
}


/*
 * This method is run on a per-connection basis in a separate thred.
 */
void proc_client(client_info *arg)
{
	int i;
	char buffer[1024];
	int ret;
	int len;
	int socklen;
	client_info *ci_self;
	client_info *ci;
	int client_sockfd;
	
	fd_set readfds;
	
	ci_self = (client_info *)arg;
	
	FD_ZERO(&readfds);
	FD_SET(ci_self->client_sockfd, &readfds);

	while (1)
	{
		ret = select(FD_SETSIZE, &readfds, (fd_set *)0, 
			(fd_set *)0, (struct timeval *) 0);
		if (ret < 1)
		{
			perror(strerror(errno));
		}
		else
		{
			ioctl(ci_self->client_sockfd, FIONREAD, &len);
			if (len > 0)
			{
				/* Read data from stream */
				socklen = sizeof(ci_self->client_address);
				len = recvfrom(ci_self->client_sockfd, buffer, sizeof(buffer), 0, 
					(struct sockaddr *)&ci_self->client_address, (socklen_t *)&socklen);
		  		buffer[len] = '\0';
		  		
		  		process_msg(buffer, ci_self->client_sockfd);	
			}
		}
	}
}


void process_msg(char *message, int self_sockfd)
{
	char buffer[1024];
	regex_t regex_quit;
	regex_t regex_nick;
	int ret;
	char newnick[20];
	client_info *ci;
	
	memset(buffer, 0, 1024);
	memset(newnick, 0, 20);
	
	/* Load client info object */
	int idx = get_client_info_idx_by_sockfd(self_sockfd);
	llist_find_data(idx, (void *)&ci, &ll_clients);
	
	/* Remove \r\n from message */
	chomp(message);
	
	/* Compile regex patterns */
	regcomp(&regex_quit, "^/quit$", REG_EXTENDED);
	regcomp(&regex_nick, "^/nick ([a-z]{1,5})$", REG_EXTENDED);
	
	/* Check if user wants to quit */
	ret = regexec(&regex_quit, message, 0, NULL, 0);
	if (ret == 0)
	{
		/* Broadcast message */
		sprintf(buffer, "User %s has left the chat server.", ci->nickname);
		send_broadcast_msg(buffer, self_sockfd);
		logline(LOG_INFO, "%s", buffer);

		/* Disconnect client from server */
		close(self_sockfd);

		/* Remove entry from linked list */
		llist_remove_data(idx, (void *)&ci, &ll_clients);
		
		pthread_exit(0);
	}

	/* Check if user wants to change nick */		
	size_t ngroups = 2;
	regmatch_t groups[2];
	ret = regexec(&regex_nick, message, ngroups, groups, 0);
	if (ret == 0)
	{
		size_t len = groups[1].rm_eo - groups[1].rm_so;
		memcpy(newnick, message + groups[1].rm_so, len);
						
		/* Change nickname */
		change_nickname(ci->nickname, newnick);
		
		/* Broadcast message */
		sprintf(buffer, "User %s is now known as %s.", ci->nickname, newnick);
		send_broadcast_msg(buffer, self_sockfd);
		logline(LOG_INFO, "%s", buffer);
		return;
	}
	
	/* Broadcast message to all clients */
	sprintf(buffer, "%s\r\n", message);
	send_broadcast_msg(buffer, self_sockfd);
	
	/* Log message on server console */
	logline(LOG_INFO, "%s: %s", ci->nickname, message);
}


/* Send received message out to all available clients, but do
 * not send it to yourself.
 */
void send_broadcast_msg(char *message, int self_sockfd)
{
	int i;
	int socklen;
	client_info *ci;
	
	for (i = 0; i < llist_get_count(&ll_clients); i++)
	{
		llist_find_data(i, (void *)&ci, &ll_clients);
		/* TODO: ci points to 0x00 --> SEGFAULT! */
		if (ci->client_sockfd != self_sockfd)
		{
			socklen = sizeof(ci->client_address);
			sendto(ci->client_sockfd, message, strlen(message), 0,
				(struct sockaddr *)&(ci->client_address), (socklen_t)socklen);
		}
	}
}





/*
 * Removes newlines \n from the char array.
 */
void chomp(char *s) 
{
	while(*s && *s != '\n' && *s != '\r') s++;

	*s = 0;
}


/*
 * Changes the nickname of an existing chat user.
 */
void change_nickname(char *oldnickname, char *newnickname)
{
	client_info *ci;
	client_info *ci_new;
	
	/* Get index of linked list element to modify */
	int idx = get_client_info_idx_by_nickname(oldnickname);

	/* Load element based on index */
	llist_find_data(idx, (void *)&ci, &ll_clients);

	/* Prepare new list entry using new nickname */
	ci_new = (client_info *)malloc(sizeof(client_info));
	strcpy(ci_new->nickname, newnickname);
	ci_new->client_sockfd = ci->client_sockfd;
	ci_new->client_address = ci->client_address;
	
	/* Update nickname */
	llist_change_data(idx, &ci_new, &ll_clients);	
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


/*
 * Gets a client_info structure by passing a socket file descriptor
 * as a query filter.
 */
int get_client_info_idx_by_sockfd(int sockfd)
{
	int i;
	client_info *ci;
	
	for (i = 0; i < llist_get_count(&ll_clients); i++)
	{
		llist_find_data(i, (void *)&ci, &ll_clients);
		if (ci->client_sockfd == sockfd)
		{
			return i;
		}
	}

	return -1;
}


/*
 * Gets a client_info structure by passing a nickname
 * as query filter.
 */
int get_client_info_idx_by_nickname(char *nickname)
{
	int i;
	client_info *ci;
	
	for (i = 0; i < llist_get_count(&ll_clients); i++)
	{
		llist_find_data(i, (void *)&ci, &ll_clients);
		if (strcmp(ci->nickname, nickname) == 0)
		{
			return i;
		}
	}

	return -1;
}


/* 
 * Display a helpful page.
 */
void display_help_page(void)
{
	show_gnu_banner();
	printf("\n");
	printf("Syntax:\n");
	printf("-------\n");
	printf("%s [OPTIONS]\n", APP_NAME);
	printf("\n");
	printf("Parameters:\n");
	printf("-----------\n");
	printf("%s currently supports the following parameters:\n\n", APP_NAME);
	printf("--ip=<ip address>, -i <ip address>         Specifies the ip address which shall be\n");
	printf("                                           used by %s. If no ip address\n", APP_NAME); 
	printf("                                           is specified, 127.0.0.1 will be used by default.\n");
	printf("--port=<port number>, -p <port number>     Specifies the TCP port to be used by %s.\n", APP_NAME);
	printf("                                           If no port is specified, port 5555 will be\n");
	printf("                                           used by default.\n");
	printf("--loglevel=<level>, -l <level>             Specifies the desired log level. The\n");
	printf("                                           following levels are supported:\n");
	printf("                                             1 = ERROR (Log errors only)\n");
	printf("                                             2 = INFO (Log additional information)\n");
	printf("                                             3 = DEBUG (Log debug level information)\n");
	printf("--version, -v                              Displays version information.\n");
	printf("--help, -h                                 Displays this help page.\n");
	printf("\n");
    printf("Bug Reports and Other Comments:\n");
    printf("-------------------------------\n");
    printf("I'm glad to receive comments and bug reports on this tool. You can best reach me\n");
    printf("by email or IM using Jabber:\n\n");
    printf("+ E-Mail:     andre.gasser@gmx.ch\n");
    printf("+ Jabber:     sh0ka@jabber.ccc.de\n");
    printf("\n");
    printf("Source Code:\n");
    printf("------------\n");
    printf("Fetch the latest version from:\n");
    printf("+ GitHub:     https://github.com/shoka/chatsrv\n\n");
}


/* 
 * Display version information.
 */
void display_version_info(void)
{
	printf("%s %s\n", APP_NAME, APP_VERSION);
	printf("Copyright (C) 2012 André Gasser\n");
	printf("License GPLv3: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}


/*
 * Display a short GNU banner.
 */
void show_gnu_banner(void)
{
	printf("%s %s  Copyright (C) 2012  André Gasser\n", APP_NAME, APP_VERSION);
    printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions.\n");
}


