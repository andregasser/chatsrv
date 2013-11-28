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
 
#include <pthread.h>
#include "bool.h"

typedef struct client_info
{
	int sockfd;
	char nickname[20];
	struct sockaddr_in address;
} client_info;

typedef struct list_entry
{
	struct client_info *client_info;
	pthread_mutex_t *mutex;
	struct list_entry *next;
} list_entry;

void llist_init(list_entry *list_start);
int llist_insert(list_entry *list_start, client_info *element);
int llist_remove_by_sockfd(list_entry *list_start, int sockfd);
list_entry* llist_find_by_sockfd(list_entry *list_start, int sockfd);
list_entry* llist_find_by_nickname(list_entry *list_start, char *nickname);
int llist_change_by_sockfd(list_entry *list_start, client_info *element, int sockfd);
int llist_show(list_entry *list_start);
int llist_get_count(list_entry *list_start);
int llist_get_nicknames(list_entry *list_start, char** nicks);
