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
#include "llist_threads.h"


int llist_init(llist_t *llistp) 
{
	int rtn;

	llistp->first = NULL;
	if ((rtn = pthread_mutex_init(&(llistp->mutex), NULL)) != 0)
		fprintf(stderr, "pthread_mutex_init error %d", rtn), exit(1);
	return 0;
}

int llist_insert_data(int index, void *datap, llist_t *llistp) 
{
	llist_node_t *cur, *prev, *new;
	int found = FALSE;

	pthread_mutex_lock(&(llistp->mutex));

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

	pthread_mutex_unlock(&(llistp->mutex));

	return 0;
}

int llist_remove_data(int index, void **datapp, llist_t *llistp) 
{
	llist_node_t *cur, *prev;

	/* Initialize to "not found" */
	*datapp = NULL;

	pthread_mutex_lock(&(llistp->mutex));

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

	pthread_mutex_unlock(&(llistp->mutex));

	return 0;
}

int llist_find_data(int index, void **datapp, llist_t *llistp) 
{
	llist_node_t *cur, *prev;

	/* Initialize to "not found" */
	*datapp = NULL;

	pthread_mutex_lock(&(llistp->mutex));

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

	pthread_mutex_unlock(&(llistp->mutex));

	return 0;
}

int llist_change_data(int index, void *datap, llist_t *llistp)
{
	llist_node_t *cur, *prev;
	int status = -1; /* assume failure */

	pthread_mutex_lock(&(llistp->mutex));

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

	pthread_mutex_unlock(&(llistp->mutex));

	return status;
}

int llist_show(llist_t *llistp)
{
	llist_node_t *cur;

	pthread_mutex_lock(&(llistp->mutex));

	printf (" Linked list contains : \n");
	for (cur = llistp->first; cur != NULL; cur = cur->nextp) 
	{
		printf ("Index: %d\tData: %s \n", cur->index, cur->datap);    
	}

	pthread_mutex_unlock(&(llistp->mutex));

	return 0;
}


