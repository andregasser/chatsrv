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
#include "rdwr.h"

#define TRUE  1
#define FALSE 0

typedef struct llist_node {
	int index;
	void *datap;
	struct llist_node *nextp;
} llist_node_t;

typedef struct llist { 
	llist_node_t *first;
	pthread_rdwr_t rwlock;
} llist_t;

int llist_init(llist_t *llistp);
int llist_insert_data(int index, void *datap, llist_t *llistp);
int llist_remove_data(int index, void **datapp, llist_t *llistp);
int llist_find_data(int index, void **datapp, llist_t *llistp);
int llist_change_data(int index, void *datap, llist_t *llistp);
int llist_show(llist_t *llistp);
int llist_get_count(llist_t *llistp);
