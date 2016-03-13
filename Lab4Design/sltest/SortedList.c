#define _GNU_SOURCE

#include <stdlib.h> 	// free
#include "string.h" 	// strcmp
#include <pthread.h>
#include "SortedList.h"

int opt_yield; 	// defined in SortedList.h 

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 *
 * Note: if (opt_yield & INSERT_YIELD)
 *		call pthread_yield in middle of critical section
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
	// Check for bad pointers 
	if(list == NULL || element == NULL)
		return; 

	// slow and fast pointer 

	// critical section 
	SortedListElement_t *p = list; 
	SortedListElement_t *q = list->next; 

	if (opt_yield & INSERT_YIELD)
		pthread_yield(); 

	// Traverse list until we hit sentinel again 
	while(q != list){
		if(strcmp(element->key, q->key) <= 0)
			break; 
		p = q; 
		q = q->next; 
	}

	// Two cases:
	// 1) element key is <= q's key; insert before q
	// 2) element key is greater than all keys; insert at end of list 

	// Rewire 4 pointers 
	element->next = q; 
	element->prev = p; 
	p->next = element; 
	q->prev = element; 
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 * Note: if (opt_yield & DELETE_YIELD)
 *		call pthread_yield in middle of critical section
 */
int SortedList_delete( SortedListElement_t *element){
	if (element == NULL)
		return 1; 

	SortedListElement_t *q = element->next; 
	// middle of critical section 
	if (opt_yield & DELETE_YIELD)
		pthread_yield();
	SortedListElement_t *p = element->prev; 

	// Check for race conditions
	if( (p->next != element) || (q->prev != element) )
		return 1; 

	q->prev = p; 
	p->next = q; 
	element->next = NULL;
	element->prev = NULL;

	// free memory? 
	free(element); 
	return 0; 
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *		call pthread_yield in middle of critical section
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
	if (list == NULL)
		return NULL; 

	SortedListElement_t *p = list->next; 
	// middle of critical section 
	if (opt_yield & SEARCH_YIELD)
		pthread_yield();

	while(p != list && p != NULL){
		if (p->key == key)
			return p; 
		p = p->next; 
	}

	// Either we have an empty list or no match in a non-empty set was found 
	return NULL; 
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *		call pthread_yield in middle of critical section
 */
int SortedList_length(SortedList_t *list){
	// Handle errors 
	if (list == NULL)
		return -1;

	// General cases
	int len = 0;
	SortedListElement_t *q = list->next; 
	SortedListElement_t *p, *r; 

	// middle of critical section 
	if (opt_yield & SEARCH_YIELD)
		pthread_yield();

	while(q != list){
		p = q->prev; 
		r = q->next; 

		// check for race conditions
		if(p->next != q || r->prev != q)
			return -1; 
		q = q->next; 	
		len++;	
	}

	return len; 
}
