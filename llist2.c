/******************************************************************************
 *    Copyright 2012 André Gasser
 *
 *    This file is part of CHATSRV.
 *
 *    CHATSRV is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    CHATSRV is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with CHATSRV. If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "llist2.h"
#include "log.h"


/*
 * Initializes the first element of the list.
 */
void llist_init(list_entry *list_start)
{
	list_start->client_info = NULL;
	list_start->next = NULL;
	list_start->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));	
	pthread_mutex_init(list_start->mutex, NULL);
}


/*
 * Inserts a new client_info element at the end of the list.
 */
int llist_insert(list_entry *list_start, client_info *element)
{
	list_entry *cur, *prev;
	int inserted = FALSE;

	cur = prev = list_start;
	
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);	
	
		/* Delete client_info data if sockfd matches */
		if (cur->client_info == NULL)
		{
			cur->client_info = element;
			pthread_mutex_unlock(cur->mutex);
			inserted = TRUE;
			break;
		}
	
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
	
		/* Load next entry */
		prev = cur;
		cur = cur->next;
	}
	
	/* During iteration through list, no existing element could be reused.
	 * We therefore need to append a new list_entry to the list.
	 */
	if (inserted == FALSE)
	{
		/* Lock last entry again */
		pthread_mutex_lock(prev->mutex);
	
		/* Create new list entry */	
		list_entry *new_entry = (list_entry *)malloc(sizeof(list_entry));
		new_entry->client_info = element;
		new_entry->next = NULL;
		new_entry->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));	
		pthread_mutex_init(new_entry->mutex, NULL);
	
		/* Append entry */
		prev->next = new_entry;
		
		/* Unlock list entry */
		pthread_mutex_unlock(prev->mutex);
	
		inserted = TRUE;
	}

	if (inserted == TRUE)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}


/*
 * Removes a client_info element by sockfd.
 */
int llist_remove_by_sockfd(list_entry *list_start, int sockfd)
{
	list_entry *cur;

	cur = list_start;
	
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);	
	
		/* Delete client_info data if sockfd matches */
		if (cur->client_info->sockfd == sockfd)
		{
			cur->client_info = NULL;
			pthread_mutex_unlock(cur->mutex);
			break;
		}
	
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
	
		/* Load next entry */
		cur = cur->next;
	}

	return 0;
}


/*
 * Find a client_info element by sockfd.
 */
list_entry* llist_find_by_sockfd(list_entry *list_start, int sockfd)
{
	list_entry *cur;

	cur = list_start;
	
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);	
	
		/* Delete client_info data if sockfd matches */
		if (cur->client_info->sockfd == sockfd)
		{
			pthread_mutex_unlock(cur->mutex);
			return cur;
		}
	
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
	
		/* Load next entry */
		cur = cur->next;
	}
	
	return NULL;
}


list_entry* llist_find_by_nickname(list_entry *list_start, char *nickname)
{
	list_entry *cur;

	cur = list_start;
	
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);	
	
		/* Delete client_info data if sockfd matches */
		if (strcmp(cur->client_info->nickname, nickname) == 0)
		{
			pthread_mutex_unlock(cur->mutex);
			return cur;
		}
	
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
	
		/* Load next entry */
		cur = cur->next;
	}
	
	return NULL;
}


/*
 * Replace a client_info element by sockfd.
 */
int llist_change_by_sockfd(list_entry *list_start, client_info *element, int sockfd)
{
	list_entry *cur;

	cur = list_start;
	
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);	
	
		/* Delete client_info data if sockfd matches */
		if (cur->client_info->sockfd == sockfd)
		{
			cur->client_info = element;
			pthread_mutex_unlock(cur->mutex);
			return 0;
		}
	
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
	
		/* Load next entry */
		cur = cur->next;
	}

	return 0;
}


/*
 * Display elements in linked list.
 */
int llist_show(list_entry *list_start)
{
	list_entry *cur;

	cur = list_start;
	
	logline(LOG_DEBUG, "---------- Client List Dump Begin ----------");
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);
		
		/* Display client info */
		if (cur->client_info != NULL)
		{
			logline(LOG_DEBUG, "sockfd = %d, nickname = %s", cur->client_info->sockfd, cur->client_info->nickname);
		}
		
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
		
		/* Load next entry */
		cur = cur->next;
	}
	logline(LOG_DEBUG, "----------- Client List Dump End -----------");
	
	return 0;
}


/*
 * Get number of client_info elements in list.
 */
int llist_get_count(list_entry *list_start)
{
	list_entry *cur;
	int count = 0;

	cur = list_start;
	
	while (cur != NULL)
	{
		/* Lock entry */
		pthread_mutex_lock(cur->mutex);		
		
		/* Increase count if client_info not null */
		if (cur->client_info != NULL)
		{
			count++;
		}
		
		/* Unlock entry */
		pthread_mutex_unlock(cur->mutex);
		
		/* Load next entry */	
		cur = cur->next;
	}
	
	return count;
}
