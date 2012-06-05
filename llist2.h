/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************
 * llist_threads.h --
 *
 * Include file for linked list with threads support
 */
#include <pthread.h>


typedef struct client_info
{
	int sockfd;
	char nickname[20];
	struct sockaddr_in address;
	pthread_mutex_t mutex;
	struct client_info *next;
} client_info;

int llist_insert(client_info **list_start, client_info *element);
int llist_remove_by_sockfd(client_info **list_start, int sockfd);
client_info* llist_find_by_sockfd(client_info *list_start, int sockfd);
client_info* llist_find_by_nickname(client_info *list_start, char *nickname);
int llist_change_by_sockfd(client_info **list_start, client_info *element, int sockfd);
int llist_show(client_info *list_start);
int llist_get_count(client_info *list_start);
