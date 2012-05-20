/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************
 * llist_threads.c --
 *
 * Linked list library with threads support
 */
#include <stdio.h>
#include <stdlib.h>
#include "llist.h"


int llist_init(llist_t *llistp) 
{
	int rtn;

	llistp->first = NULL;
	if ((rtn = pthread_rdwr_init_np(&(llistp->rwlock), NULL)) != 0)
		fprintf(stderr, "pthread_rdwr_init_np error %d", rtn), exit(1);

	return 0;
}

int llist_insert_data(int index, void *datap, llist_t *llistp) 
{
	llist_node_t *cur, *prev, *new;
	int found = FALSE;

	pthread_rdwr_wlock_np(&(llistp->rwlock));

	for (cur = prev = llistp->first; cur != NULL; prev = cur, cur = cur->nextp) 
	{
		if (cur->index == index) 
		{
			free(cur->datap);
			cur->datap = datap;
			found = TRUE;
			break;
		}
		else if (cur->index > index)
		{
			break;
		}
	}

	if (!found) 
	{
		new = (llist_node_t *)malloc(sizeof(llist_node_t));
		new->index = index;
		new->datap = datap;
		new->nextp = cur;
		if (cur == llistp->first)
			llistp->first = new;
		else
			prev->nextp = new;
	}

	pthread_rdwr_wunlock_np(&(llistp->rwlock));

	return 0;
}

int llist_remove_data(int index, void **datapp, llist_t *llistp) 
{
	llist_node_t *cur, *prev;

	/* Initialize to "not found" */
	*datapp = NULL;

	pthread_rdwr_wlock_np(&(llistp->rwlock));

	for (cur = prev = llistp->first; cur != NULL; prev = cur, cur = cur->nextp)
	{
		if (cur->index == index) 
		{
			*datapp = cur->datap;
			prev->nextp = cur->nextp;
			free(cur);
			break;
		}
		else if (cur->index > index)
		{
			break;
		}
	}

	pthread_rdwr_wunlock_np(&(llistp->rwlock));

	return 0;
}

int llist_find_data(int index, void **datapp, llist_t *llistp) 
{
	llist_node_t *cur, *prev;

	/* Initialize to "not found" */
	*datapp = NULL;

	pthread_rdwr_rlock_np(&(llistp->rwlock));

	/* Look through index for our entry */
	for (cur = prev = llistp->first; cur != NULL; prev = cur, cur = cur->nextp) 
	{
		if (cur->index == index) 
		{
			*datapp = cur->datap; 
			break;
		}
		else if (cur->index > index)
		{
			break;
		}
	}

	pthread_rdwr_runlock_np(&(llistp->rwlock));

	return 0;
}

int llist_change_data(int index, void *datap, llist_t *llistp)
{
	llist_node_t *cur, *prev;
	int status = -1; /* assume failure */

	pthread_rdwr_wlock_np(&(llistp->rwlock));

	for (cur = prev = llistp->first; cur != NULL; prev = cur, cur = cur->nextp) 
	{
		if (cur->index == index) 
		{
			cur->datap = datap;
			prev->nextp = cur->nextp;
			free(cur);
			status = 0;
			break;
		}
		else if (cur->index > index)
		{
			break;
		}
	}

	pthread_rdwr_wunlock_np(&(llistp->rwlock));

	return status;
}

int llist_show(llist_t *llistp)
{
	llist_node_t *cur;

	pthread_rdwr_rlock_np(&(llistp->rwlock));

	printf (" Linked list contains : \n");
	for (cur = llistp->first; cur != NULL; cur = cur->nextp) 
	{
		printf ("Index: %d\tData: %s \n", cur->index, cur->datap);    
	}

	pthread_rdwr_runlock_np(&(llistp->rwlock));

	return 0;
}


int llist_get_count(llist_t *llistp)
{
	llist_node_t *cur;
	int count = 0;
	
	pthread_rdwr_rlock_np(&(llistp->rwlock));
	
	for (cur = llistp->first; cur != NULL; cur = cur->nextp)
	{
		count++;
	}	
	
	pthread_rdwr_runlock_np(&(llistp->rwlock));
	
	return count;
}
