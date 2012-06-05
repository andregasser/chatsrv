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
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <regex.h>
#include <getopt.h>
#include <limits.h>
#include "log.h"
#include "llist2.h"


/* Define some constants */
#define APP_NAME        "CHATSRV" /* Name of applicaton */
#define APP_VERSION     "0.3"     /* Version of application */
#define MAX_THREADS     1000      /* Max. number of concurrent chat sessions */
#define TRUE            1
#define FALSE           0

/* Typedefs */
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
cmd_params *params;
client_info *list_client_info;

/* TODO: Use mutex to access this variable */
int curr_thread_count = 0;


/* Function prototypes */
int startup_server(void);
int parse_cmd_args(int *argc, char *argv[]);
void proc_client(int *arg);
void process_msg(char *message, int self_sockfd);
void send_broadcast_msg(char* format, ...);
void send_private_msg(char* nickname, char* format, ...);
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
	struct sockaddr_in client_address;
	int client_sockfd = 0;
	pthread_t threads[MAX_THREADS];
			
	/* Parse commandline args */
	params = malloc(sizeof(cmd_params));
	ret = parse_cmd_args(&argc, argv);
	if (params->help)
	{
		display_help_page();
		exit(0);
	}
	if (params->version)
	{
		display_version_info();
		exit(0);
	}
	if (ret < 0)
	{
		if (ret == -2)
			logline(LOG_ERROR, "Error: Invalid port range specified (-p)");
		if (ret == -6)
			logline(LOG_ERROR, "Error: Invalid log level option specified (-l).");
		logline(LOG_ERROR, "Use the -h option if you need help.");
		exit(ret);
	}
	
	/* Set log level */
	switch (params->loglevel)
	{
		case 1: set_loglevel(LOG_ERROR); break;
		case 2: set_loglevel(LOG_INFO); break;
		case 3: set_loglevel(LOG_DEBUG); break;
		default: set_loglevel(LOG_ERROR);
	}

	/* Setup signal handler */
	signal(SIGINT, shutdown_server);
	signal(SIGTERM, shutdown_server);
	
	/* Show banner and stuff */
	show_gnu_banner();	

	/* Startup the server listener */
	if (startup_server() < 0)
	{
		logline(LOG_ERROR, "Error during server startup. Please consult debug log for details.");
		exit(-1);
	}
	
	/* Post ready message */
	logline(LOG_INFO, "Server listening on %s, port %d", params->ip, params->port);
	switch (params->loglevel)
	{
		case LOG_ERROR: logline(LOG_INFO, "Log level set to ERROR"); break;
		case LOG_INFO: logline(LOG_INFO, "Log level set to INFO"); break;
		case LOG_DEBUG: logline(LOG_INFO, "Log level set to DEBUG"); break;
		default: logline(LOG_INFO, "Unknown log level specified"); break;
	}
	
	/* Handle connections */
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
			if (curr_thread_count < MAX_THREADS)
			{
				/* Prepare client infos in handy structure */
				client_info *ci = (client_info *)malloc(sizeof(client_info));
				ci->sockfd = client_sockfd;
				ci->address = client_address;
				sprintf(ci->nickname, "anonymous_%d", client_sockfd);
				
				/* Add client info to linked list */
				llist_insert(&list_client_info, ci);
				llist_show(list_client_info);
				
				/* Pass client info and invoke new thread */	
				ret = pthread_create(&threads[curr_thread_count],
					NULL,
					(void *)&proc_client, 
					(void *)&client_sockfd); /* only pass socket id ? */
			
				if (ret == 0)
				{
					pthread_detach(threads[curr_thread_count]);
					curr_thread_count++;
										
					/* Notify server and clients */
					logline(LOG_INFO, "User %s joined the chat.", ci->nickname);	
					logline(LOG_DEBUG, "main(): Connections used: %d of %d", curr_thread_count, MAX_THREADS);
					send_broadcast_msg("User %s joined the chat.\r\n", ci->nickname);
				}
				else
				{
					free(ci);
					close(client_sockfd);
				}
			}
			else
			{
				logline(LOG_ERROR, "Max. connections reached. Connection limit is %d. Connection dropped.", MAX_THREADS);
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
	
	free(params);

	return 0;
}


/* 
 * Startup the server listener.
 */
int startup_server(void)
{
	int optval = 1;
	
	/* Create socket */
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0)
	{
		logline(LOG_DEBUG, "Error calling setsockopt(): %s", strerror(errno));
		return -1;
	}

	/* Name the socket */
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(params->ip);
	server_address.sin_port = htons(params->port);
	server_len = sizeof(server_address);
	if (bind(server_sockfd, (struct sockaddr *)&server_address, server_len) != 0)
	{
		logline(LOG_DEBUG, "Error calling bind(): %s", strerror(errno));
		return -2;
	}

	/* Create a connection queue and wait for incoming connections */
	if (listen(server_sockfd, 5) != 0)
	{
		logline(LOG_DEBUG, "Error calling listen(): %s", strerror(errno));
		return -3;
	}

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
			case 'i': params->ip = optarg; break;
			case 'p':
				params->port = atoi(optarg);
				if ((params->port < 1) || (params->port > 65535))
					return -2;
				break;
			case 'h': params->help = 1; break;
			case 'v': params->version = 1; break;
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
void proc_client(int *arg)
{
	char buffer[1024];
	char message[1024];
	int ret = 0;
	int len = 0;
	int socklen = 0;
	client_info *ci;
	fd_set readfds;
	int sockfd = 0;
	
	/* Load associated client info */
	sockfd = *arg;
	ci = llist_find_by_sockfd(list_client_info, sockfd);
	
	memset(message, 0, 1024);
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	while (1)
	{
		ret = select(FD_SETSIZE, &readfds, (fd_set *)0, 
			(fd_set *)0, (struct timeval *) 0);
		if (ret == -1)
		{
			logline(LOG_ERROR, "Error calling select() on thread.");
			perror(strerror(errno));
		}
		else
		{
			/* Read data from stream */
			memset(buffer, 0, 1024);
			socklen = sizeof(ci->address);
			len = recvfrom(sockfd, buffer, sizeof(buffer), 0, 
				(struct sockaddr *)&ci->address, (socklen_t *)&socklen);

			logline(LOG_DEBUG, "proc_client(): Receive buffer contents = %s", buffer);

			/* Copy receive buffer to message buffer */
			strcat(message, buffer);
			
			/* Check if message buffer contains a full message. A full message
			 * is recognized by its terminating \n character. If a message
			 * is found, process it and clear message buffer afterwards.
			 */
			char *pos = strstr(message, "\n");
			if (pos != NULL)
			{		  		
				chomp(message);
				logline(LOG_DEBUG, "proc_client(): Message buffer contents = %s", message);
				logline(LOG_DEBUG, "proc_client(): Complete message received.");

		  		/* Process message */
		  		process_msg(message, sockfd);
		  		memset(message, 0, 1024);	
			}
			else
			{
				logline(LOG_DEBUG, "proc_client(): Message buffer contents = %s", message);
				logline(LOG_DEBUG, "proc_client(): Message still incomplete.");
			}
		}
	}
}


void process_msg(char *message, int self_sockfd)
{
	char buffer[1024];
	regex_t regex_quit;
	regex_t regex_nick;
	regex_t regex_msg;
	regex_t regex_me;
	int ret;
	char newnick[20];
	char oldnick[20];
	char priv_nick[20];
	client_info *ci = NULL;
	client_info *priv_ci = NULL;
	int processed = FALSE;
	size_t ngroups = 0;
	size_t len = 0;
	regmatch_t groups[3];
	
	memset(buffer, 0, 1024);
	memset(newnick, 0, 20);
	memset(oldnick, 0, 20);
	memset(priv_nick, 0, 20);
	
	/* Load client info object */
	ci = llist_find_by_sockfd(list_client_info, self_sockfd);
	
	/* Remove \r\n from message */
	chomp(message);
	
	/* Compile regex patterns */
	regcomp(&regex_quit, "^/quit$", REG_EXTENDED);
	regcomp(&regex_nick, "^/nick ([a-zA-Z0-9_]{1,19})$", REG_EXTENDED);
	regcomp(&regex_msg, "^/msg ([a-zA-Z0-9_]{1,19}) (.*)$", REG_EXTENDED);
	regcomp(&regex_me, "^/me (.*)$", REG_EXTENDED);

	/* Check if user wants to quit */
	ret = regexec(&regex_quit, message, 0, NULL, 0);
	if (ret == 0)
	{
		/* Notify */
		send_broadcast_msg("User %s has left the chat server.\r\n", ci->nickname);
		logline(LOG_INFO, "User %s has left the chat server.", ci->nickname);
		curr_thread_count--;
		logline(LOG_DEBUG, "process_msg(): Connections used: %d of %d", curr_thread_count, MAX_THREADS);

		/* Disconnect client from server */
		close(self_sockfd);

		/* Remove entry from linked list */
		llist_remove_by_sockfd(&list_client_info, self_sockfd);

		/* Free memory */
		regfree(&regex_quit);
		regfree(&regex_nick);

		/* Terminate this thread */		
		pthread_exit(0);
	}

	/* Check if user wants to change nick */		
	ngroups = 2;
	ret = regexec(&regex_nick, message, ngroups, groups, 0);
	if (ret == 0)
	{
		processed = TRUE;
		
		/* Extract nickname */
		len = groups[1].rm_eo - groups[1].rm_so;
		strncpy(newnick, message + groups[1].rm_so, len);
		strcpy(oldnick, ci->nickname);
		
		strcpy(buffer, "User ");
		strcat(buffer, oldnick);
		strcat(buffer, " is now known as ");
		strcat(buffer, newnick);
							
		/* Change nickname */
		change_nickname(oldnick, newnick);
		
		/* Broadcast message */
		send_broadcast_msg("%s\r\n", buffer);
		logline(LOG_INFO, buffer);
	}
	
	/* Check if user wants to transmit a private message to another user */
	ngroups = 3;
	ret = regexec(&regex_msg, message, ngroups, groups, 0);
	if (ret == 0)
	{
		processed = TRUE;
		
		/* Extract nickname and private message */
		len = groups[1].rm_eo - groups[1].rm_so;
		memcpy(priv_nick, message + groups[1].rm_so, len);
		len = groups[2].rm_eo - groups[2].rm_so;
		memcpy(buffer, message + groups[2].rm_so, len);
		
		/* Check if nickname exists. If yes, send private message to user. 
		 * If not, ignore message.
		 */
		priv_ci = llist_find_by_nickname(list_client_info, priv_nick);
		if (priv_ci != NULL)
		{
			send_private_msg(priv_nick, "%s: %s\r\n", ci->nickname, buffer);
			logline(LOG_INFO, "Private message from %s to %s: %s", ci->nickname, priv_nick, buffer);
		}
	}
	
	/* Check if user wants to say something about himself */
	ngroups = 2;
	ret = regexec(&regex_me, message, ngroups, groups, 0);
	if (ret == 0)
	{
		processed = TRUE;
		
		strcpy(buffer, ci->nickname);
		
		/* Prepare message */
		len = groups[1].rm_eo - groups[1].rm_so;
		strcat(buffer, " ");
		strncat(buffer, message + groups[1].rm_so, len);
			
		/* Broadcast message */
		send_broadcast_msg("%s\r\n", buffer);
		logline(LOG_INFO, buffer);
	}
	
	/* Broadcast message */
	if (processed == FALSE)
	{
		send_broadcast_msg("%s: %s\r\n", ci->nickname, message);
		logline(LOG_INFO, "%s: %s", ci->nickname, message);
	}

	/* Dump current user list */
	llist_show(list_client_info);
	
	/* Free memory */
	regfree(&regex_quit);
	regfree(&regex_nick);
	regfree(&regex_msg);
}


/* Send received message out to all available clients, but do
 * not send it to yourself.
 */
void send_broadcast_msg(char* format, ...)
{
	int socklen = 0;
	client_info *cur = NULL;
	va_list args;
	char buffer[1024];
		
	memset(buffer, 0, 1024);

	/* Prepare message */
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	
	cur = list_client_info;
	while (cur != NULL)
	{
		/* Send message to client */
		socklen = sizeof(cur->address);
		sendto(cur->sockfd, buffer, strlen(buffer), 0,
			(struct sockaddr *)&(cur->address), (socklen_t)socklen);
		
		/* Load next index */
		cur = cur->next;
	}
}


/*
 * Sends a private message to a user.
 */
void send_private_msg(char* nickname, char* format, ...)
{
	int socklen = 0;
	client_info *cur = NULL;
	va_list args;
	char buffer[1024];
		
	memset(buffer, 0, 1024);

	/* Prepare message */
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	
	cur = llist_find_by_nickname(list_client_info, nickname);

	/* Send message to client */
	socklen = sizeof(cur->address);
	sendto(cur->sockfd, buffer, strlen(buffer), 0,
		(struct sockaddr *)&(cur->address), (socklen_t)socklen);
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
	client_info *ci = NULL;
	client_info *ci_new = NULL;
	int idx = 0;
	
	logline(LOG_DEBUG, "change_nickname(): oldnickname = %s, newnickname = %s", oldnickname, newnickname);
	
	/* Load client_info element */
	ci = llist_find_by_nickname(list_client_info, oldnickname);
	
	logline(LOG_DEBUG, "change_nickname(): client_info found. client_info->nickname = %s", ci->nickname);
	
	/* Prepare new list entry using new nickname */
	ci_new = (client_info *)malloc(sizeof(client_info));
	strcpy(ci_new->nickname, newnickname);
	ci_new->sockfd = ci->sockfd;
	ci_new->address = ci->address;
	
	/* Update nickname */
	llist_change_by_sockfd(&list_client_info, ci_new, ci->sockfd);
}


/*
 * Shuts down the server properly by freeing all allocated resources.
 */
void shutdown_server(int sig)
{
	int i = 0;
	int *client_sockfd = NULL;


	if (sig == SIGINT)
	{
		logline(LOG_INFO, "Server shutdown requested per SIGINT. Performing cleanup ops now.");
		
		/* Close all socket connections immediately */
		logline(LOG_INFO, "Closing socket connections...");		
		
		/* TODO: Code proper shutdown and list cleanup */
		//for (i = 0; i < llist_get_count(list_client_info); i++)
		//{
		//	llist_find_data(i, (void *)&client_sockfd, &ll_clients);
		//	close(*client_sockfd);
		//}
		
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


