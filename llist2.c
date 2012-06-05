/*
 * llist_threads.c --
 *
 * Linked list library with threads support
 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "llist2.h"
#include "log.h"


/*
 * Inserts a new client_info element at the end of the list.
 */
int llist_insert(client_info **list_start, client_info *element)
{
	client_info *cur, *prev;

	cur = prev = *list_start;
	
	if (cur == NULL)
	{
		*list_start = element;
	}
	else
	{
		
		while (cur != NULL)
		{
			prev = cur;
			cur = cur->next;
		}

		/* Lock */
		pthread_mutex_lock(&(prev->mutex));

		/* Modify */		
		prev->next = element;
		prev->next->next = NULL;	
	
		/* Unlock */
		pthread_mutex_unlock(&(prev->mutex));
	}

	return 0;
}


/*
 * Removes a client_info element by sockfd.
 */
int llist_remove_by_sockfd(client_info **list_start, int sockfd)
{
	client_info *cur, *prev;
	pthread_mutex_t mutex_ptr;

	cur = prev = *list_start;
	while (cur != NULL)
	{
		if (cur->sockfd == sockfd)
		{
			/* Special treatment for first element in list */
			if (cur == *list_start)
			{
				logline(LOG_DEBUG, "llist_remove_by_sockfd(): Remove first element");
				
				/* Lock */
				pthread_mutex_lock(&((*list_start)->mutex));
				
				/* Since we're deleting the only element in the list and a lock
				 * is required to delete it, we must save the mutex location first
				 * in order to be able to unlock it after the removal operation.
				 */
				mutex_ptr =(*list_start)->mutex;
				
				/* Modify */
				*list_start = cur->next;
				
				/* Unlock */
				pthread_mutex_unlock(&mutex_ptr);
								
				free(cur);
				break;			
			}
			else
			{
				logline(LOG_DEBUG, "llist_remove_by_sockfd(): Remove 2..nth element");
				
				/* Lock */
				pthread_mutex_lock(&(cur->mutex));
				pthread_mutex_lock(&(prev->mutex));

				/* Modify */			
				prev->next = cur->next;
			
				/* Unlock */
				pthread_mutex_unlock(&(prev->mutex));
				pthread_mutex_unlock(&(prev->mutex));
			
				free(cur);
				break;
			}	
		}
		prev = cur;
		cur = cur->next;
	}
	
	return 0;
}


/*
 * Find a client_info element by sockfd.
 */
client_info* llist_find_by_sockfd(client_info *list_start, int sockfd)
{
	client_info *cur, *prev;

	cur = prev = list_start;
	while (cur != NULL)
	{
		if (cur->sockfd == sockfd)
		{
			return cur;	
		}
		prev = cur;
		cur = cur->next;
	}
	
	return NULL;
}


client_info* llist_find_by_nickname(client_info *list_start, char *nickname)
{
	client_info *cur, *prev;

	cur = prev = list_start;
	while (cur != NULL)
	{
		if (strcmp(cur->nickname, nickname) == 0)
		{
			return cur;	
		}
		prev = cur;
		cur = cur->next;
	}
	
	return NULL;
}


/*
 * Replace a client_info element by sockfd.
 */
int llist_change_by_sockfd(client_info **list_start, client_info *element, int sockfd)
{
	client_info *cur, *prev;

	cur = prev = *list_start;
	while (cur != NULL)
	{
		if (cur->sockfd == sockfd)
		{
			/* Special treatment for first element in list */
			if (cur == *list_start)
			{
				logline(LOG_DEBUG, "llist_change_by_sockfd(): Changing first entry in list");
				
				/* Lock */
				pthread_mutex_lock(&(cur->mutex));
				
				/* Modify */
				*list_start = element;
				element->next = cur->next;
				
				/* Unlock */
				pthread_mutex_unlock(&(cur->mutex));
				
				free(cur);
				break;			
			}
			else
			{
				logline(LOG_DEBUG, "llist_change_by_sockfd(): Changing first 2..nth entry in list");
				
				/* Lock */
				pthread_mutex_lock(&(prev->mutex));
							
				prev->next = element;
				element->next = cur->next;
			
				/* Unlock */
				pthread_mutex_unlock(&(prev->mutex));
							
				free(cur);
				break;	
			}
		}
		prev = cur;
		cur = cur->next;
	}
	
	return 0;
}


/*
 * Display elements in linked list.
 */
int llist_show(client_info *list_start)
{
	client_info *cur;

	cur = list_start;
	
	logline(LOG_DEBUG, "---------- Client List Dump Begin ----------");
	while (cur != NULL)
	{
		logline(LOG_DEBUG, "sockfd = %d, nickname = %s", cur->sockfd, cur->nickname);
		cur = cur->next;
	}
	logline(LOG_DEBUG, "----------- Client List Dump End -----------");
	
	return 0;
}


/*
 * Get number of client_info elements in list.
 */
int llist_get_count(client_info *list_start)
{
	client_info *cur;
	int count = 0;

	cur = list_start;
	
	while (cur != NULL)
	{
		count++;
		cur = cur->next;
	}
	
	return count;
}
