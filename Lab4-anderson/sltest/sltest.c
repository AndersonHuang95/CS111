// CS111 Winter 2016 
// Eggert
// Lab 4 Part 2 - Parallel Updates to complex data structures  
// Anderson Huang 

#define _GNU_SOURCE

#include <unistd.h> 	
#include <errno.h> 
#include <getopt.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <time.h>
#include <pthread.h>
#include <math.h> 
#include <string.h>
#include "SortedList.h"

#define DEFAULT_SYNC 'd'
#define MAX_KEY_LENGTH 16 

// Option flags for program
// Default # of threads and iterations and lists is 1 
int n_threads = 1; 
int n_iterations = 1; 
int n_lists = 1; 
int correct_flag = 0;
int opt_yield = 0; 	// defined in SortedList.h, initialized here 
char sync_option = DEFAULT_SYNC; 

// locking mechanisms
pthread_mutex_t *lock; 
volatile int *spinlock; 

static struct option long_options[] = {
	{"threads", 	optional_argument, 	0, 				't'},
	{"iterations", 	optional_argument, 	0, 				'i'},
	{"yield", 		optional_argument, 	0,				'y'},
	{"sync", 		optional_argument, 	0, 				's'},
	{"lists", 		optional_argument, 	0, 				'l'},
	{0,				0,				   	0,				0}
};


// Struct to hold more than one arg; used for pthread_create fxn call 
typedef struct list_args{
	SortedList_t *sent_arr; 
	SortedListElement_t **list_elements; 
	int thread_no; 
	int lock_no;
} list_args_t; 

// Utility function signatures 
unsigned long hash(const char *str);
static char *rand_string(char *str);

// inserts, retrieves list length, looks up elem just inserted, deletes returned element
void * unsynchronizedListFxn (void * args){
	// Recast original type 
	list_args_t *info = (list_args_t *) args; 
	if (info == NULL)
		return NULL; 

	int c, start; 
	start = info->thread_no * n_iterations;
	// Each thread performs on specific portion of created list elements s
	for(c = start; c < start + n_iterations; c++){
		unsigned long list_no = hash(info->list_elements[c]->key) % n_lists; // # between 0 and (n_lists - 1)
		SortedList_insert(&(info->sent_arr[list_no]), info->list_elements[c]);
	}

	int len = 0;
	for(c = 0; c < n_lists; c++){
		len += SortedList_length(&(info->sent_arr[c]));
	}

	for(c = start; c < start + n_iterations; c++){
		unsigned long list_no = hash(info->list_elements[c]->key) % n_lists; // # between 0 and (n_lists - 1)
		SortedListElement_t *target = SortedList_lookup(&(info->sent_arr[list_no]), info->list_elements[c]->key);
		SortedList_delete(target);
	}

	return NULL;
}

// Synchronized version of listFxn 
void * mutexListFxn (void * args){
	// Recast original type 
	list_args_t *info = (list_args_t *) args; 
	if (info == NULL)
		return NULL; 

	int c, start; 
	start = info->thread_no * n_iterations;
	// Each thread performs on specific portion of created list elements 

	// 1) insert 
	for(c = start; c < start + n_iterations; c++){
		unsigned long list_no = hash(info->list_elements[c]->key) % n_lists; // # between 0 and (n_lists - 1)
		pthread_mutex_lock(&(lock[list_no]));
		SortedList_insert(&(info->sent_arr[list_no]), info->list_elements[c]);
		pthread_mutex_unlock(&(lock[list_no]));
	}

	// 2) length 
	int len = 0;
	for(c = 0; c < n_lists; c++){
		pthread_mutex_lock(&lock[c]);
		len += SortedList_length(&(info->sent_arr[c]));
		pthread_mutex_unlock(&lock[c]);
	}
	// 3 & 4) lookup & delete 
	for(c = start; c < start + n_iterations; c++){
		unsigned long list_no = hash(info->list_elements[c]->key) % n_lists; // # between 0 and (n_lists - 1)
		pthread_mutex_lock(&(lock[list_no]));
		SortedListElement_t *target = SortedList_lookup(&(info->sent_arr[list_no]), info->list_elements[c]->key);
		SortedList_delete(target);
		pthread_mutex_unlock(&(lock[list_no]));
	}

	return NULL;
}

// Synchronized version of listFxn 
void * lockListFxn (void * args){
	// Recast original type 
	list_args_t *info = (list_args_t *) args; 
	if (info == NULL)
		return NULL; 

	int c, start; 
	start = info->thread_no * n_iterations;
	// Each thread performs on specific portion of created list elements 

	// 1) insert 
	for(c = start; c < start + n_iterations; c++){
		unsigned long list_no = hash(info->list_elements[c]->key) % n_lists; // # between 0 and (n_lists - 1)
		while (__sync_lock_test_and_set(&spinlock[list_no], 1)) 
			while (spinlock[list_no])
				;	// busy wait 
		SortedList_insert(&(info->sent_arr[list_no]), info->list_elements[c]);	
		__sync_lock_release(&spinlock[list_no]);
	}

	// 2) length 
	int len = 0; 
	for (c = 0; c < n_lists; c++){
		while (__sync_lock_test_and_set(&spinlock[c], 1)) 
			while (spinlock[c])
				;	// busy wait 
		len += SortedList_length((&info->sent_arr[c]));
		__sync_lock_release(&spinlock[c]);
	}

	// 3 & 4) lookup & delete 
	for(c = start; c < start + n_iterations; c++){
		unsigned long list_no = hash(info->list_elements[c]->key) % n_lists; // # between 0 and (n_lists - 1)
		while (__sync_lock_test_and_set(&spinlock[list_no], 1)) 
			while (spinlock[list_no])
				;	// busy wait 
		SortedListElement_t *target = SortedList_lookup(&(info->sent_arr[list_no]), info->list_elements[c]->key);
		SortedList_delete(target);
		__sync_lock_release(&spinlock[list_no]);
	}

	return NULL;
}

// Utility function to sum second and nanosecond fields of timespec
long long addspec (struct timespec *ts)
{
	// Check if ts is nullptr here 
	long long sec = (long long) ts->tv_sec; 
	long long nsec = (long long) ts->tv_nsec;
	return (sec * pow(10, 9) + nsec); 
}

/****** Utility function to generate a random string 
Taken from codereview.stackexchange.com ... 
Credits: William Morris ***************************/
static char *rand_string(char *str)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t size = (rand() % MAX_KEY_LENGTH) + 1; 
    size_t n; 
    for (n = 0; n < size; n++) {
        int key = rand() % (int) (sizeof(charset) - 1);
        str[n] = charset[key];
    }
    str[size] = '\0';
    return str;
}

/*********** Utility function to hash a string 
Taken from stackoverflow.com .... 
Credits to cnicutar & Dan Bernstein for djb2 hash *********/
unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int main(int argc, char *argv[]){
	// Randomize seed 
	srand(time(NULL)); 

	// function pointer variable, used to choose sychronized thread execution 
	// default is unsynchronized 
	void * (*listFunction)(void *) = unsynchronizedListFxn;

	// timer variables
	struct timespec t_start; 
	struct timespec t_end; 

	// Error checker
	int n_errors = 0;

	// Command line parsing 
	int optchar; // stores value returned by getopt_long
	int long_index; // stores index of current option in long_options array  
	// See if user provided any arguments for threads or iterations 
	while((optchar = getopt_long(argc, argv, "", long_options, &long_index)) != -1){
		switch(optchar){
			case 't':
				if (optarg)
					n_threads = atoi(optarg); 
				break;
			case 'i':
				if (optarg)
					n_iterations = atoi(optarg); 
				break;
			case 'y':{
				// Handle multiple option args passed into --yield using strpbrk 
				// char *yield_opts;
				char yield_opt_args[] = "ids";
				char *pch; 
				// if nothing was passed in to --yield, break out 
				if (optarg)
					 while( (pch = strpbrk(optarg, yield_opt_args)) != NULL ){
					 	if (*pch == 'i')
					 		opt_yield |= 0x1; 
					 	if (*pch == 'd')
					 		opt_yield |= 0x2; 
					 	if (*pch == 's')
					 		opt_yield |= 0x4; 
					 	optarg++; // Advance in the array but check for end of array 
					 }
				break;		 
			}
			case 's': 
				if (optarg)
					sync_option = optarg[0]; // only the first letter is significant 
				if (sync_option == 'm')
					listFunction = mutexListFxn;
				if (sync_option == 's')
					listFunction = lockListFxn;
				break;
			case 'l':
				if (optarg)
					n_lists = atoi(optarg);
				break;
			case '?':
			default:
				n_errors++;
		} 
	}

	// Error check 
	if(n_threads < 1)
		fprintf(stderr, "Number of threads cannot be negative or zero\n"); 
	if(n_iterations < 1)
		fprintf(stderr, "Number of iterations cannot be negative or zero\n"); 
	if(n_lists < 1)
		fprintf(stderr, "Number of iterations cannot be negative or zero\n"); 

	// Initialize empty list(s) & synchronization objects 
	int i; // Loop variable
	SortedList_t sentinel[n_lists];
	for(i = 0; i < n_lists; i++){
		sentinel[i].prev = &sentinel[i];
		sentinel[i].next = &sentinel[i]; 
	}

	spinlock = (int *) malloc(sizeof(int) * n_lists); 					  // Creates array of (n_lists) ints
	lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * n_lists); // Creates array of (n_lists) mutex 
	if(!lock){
		fprintf(stderr, "Failed to dynamically allocate memory.\n"); 
		exit(1); 
		// n_errors++; 
	}
	// initialize the locks 
	for(i = 0; i < n_lists; i++){
		if (pthread_mutex_init(&(lock[i]), NULL) != 0){
			printf("Mutex initialization failed\n"); 
			n_errors++; 
		}
	}

	// Create all list elements dynamically 
	int n_elements = n_threads * n_iterations;
	SortedListElement_t *sle_arr[n_elements];
	char *rand_arr[n_elements]; 
	for(i = 0; i < n_elements; i++){
		SortedListElement_t *elem = (SortedListElement_t *) malloc(sizeof(SortedListElement_t));
		if (!elem){
			fprintf(stderr, "Failed to dynamically allocate memory.\n"); 
			exit(1); 
			// n_errors++; 
		}
		char* rand_key = (char *) malloc(sizeof(char) * MAX_KEY_LENGTH);
		rand_arr[i] = rand_key; 
		if (!rand_key){
			fprintf(stderr, "Failed to dynamically allocate memory.\n"); 
			exit(1); 
		}
		rand_string(rand_key); 
		elem->key = rand_key; 
		sle_arr[i] = elem; 
	}

	pthread_t tids[n_threads]; 			// Array to hold thread ids 
	list_args_t targs_arr[n_threads]; 	// Array to hold thread structs with args to pass 
	int retVal;
		
	// Record start time with high resolution 
	if (clock_gettime(CLOCK_MONOTONIC, &t_start)){ 
		fprintf(stderr, "clock_gettime failed.");
		n_errors++;
	}

	// Create n_threads threads and call listFunction(see above)
	for(i = 0; i < n_threads; i++){
		// hash by string to find an appropiate list header 
		unsigned long list_no = hash(sle_arr[i]->key) % n_lists; // # between 0 and (n_lists - 1)
		targs_arr[i].sent_arr = sentinel;	
		// Assign the corresponding mutex & spinlock associated with list head 
		// pointer to all list elements 
		targs_arr[i].list_elements = sle_arr; 
		// Each thread assigned a number between 0 & (n_threads - 1)
		targs_arr[i].thread_no = i; 
		targs_arr[i].lock_no = list_no;
		retVal = pthread_create(&tids[i], NULL, listFunction, &targs_arr[i]); 
		if (retVal){
			fprintf(stderr, "An error occured during thread creation. The error code is %d.\n", retVal);
			n_errors++;		
		}
	} 

	// Make main function wait for all threads 
	for(i = 0; i < n_threads; i++){
		retVal = pthread_join(tids[i], NULL);
		if (retVal){
			fprintf(stderr, "An error occured while joining threads. The error code is %d.\n", retVal);
			n_errors++;
		}
	}

	// Record end time with high resolution 
	if(clock_gettime(CLOCK_MONOTONIC, &t_end)){
		fprintf(stderr, "clock_gettime failed.");
		n_errors++;
	}

	unsigned long long n_operations = n_threads * ( (n_iterations * n_iterations) + 2 * n_iterations);
	printf("%d threads x %d iterations for length & delete x %d/2 avg len for insert & lookup  = %llu operations\n", n_threads, n_iterations, n_iterations, n_operations);

	// Check that the sorted linked list has been restored to 0 length after all threads have operated on it 
	int len = 0;
	for(i = 0; i < n_lists; i++)
		// All threads have finished; no race conditions present, no need for locks
		len += SortedList_length(&(sentinel[i]));
	if(len){
		fprintf(stderr, "ERROR: Final sorted list length = %d.\n", len); 
		n_errors++;
	}

	// Calculate total elapsed time in threads and output
	long long total_elapsed_time = addspec(&t_end) - addspec(&t_start); 
	printf("Elapsed time: %lld ns\n", total_elapsed_time); 
	printf("Average time per operation: %lld ns\n", total_elapsed_time/n_operations);
	
	for(i = 0; i < n_lists; i++){
		pthread_mutex_destroy(&(lock[i])); 
	}

	// Free memory 
	free(lock); 
	free(spinlock);

	// free random strings here
	for(i = 0; i < n_elements; i++)
		free(rand_arr[i]);

	return n_errors; 
}
