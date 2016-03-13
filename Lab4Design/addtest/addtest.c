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
#include <limits.h> 	

#define DEFAULT_SYNC 'd'
// counter
static long long count = 0; 
// Option flags for program
int correct_flag = 0;
int opt_yield = 0; 
char sync_option = DEFAULT_SYNC;

// Error checker
int n_errors = 0;
// For Artifact correction 
struct timespec *thread_create_start; 
struct timespec *thread_create_end; 
long long smallest_start_time = LLONG_MAX; 
long long largest_end_time = 0;

// locking mechanism 
static pthread_mutex_t lock; 
volatile int spinlock; 

static struct option long_options[] = {
	{"threads", 	optional_argument, 	0, 				't'},
	{"iterations", 	optional_argument, 	0, 				'i'},
	{"yield", 		optional_argument, 	0,				'y'},
	{"sync", 		optional_argument, 	0, 				's'},
	{"correct",		no_argument,		&correct_flag, 	1},
	{0,				0,				   	0,				0}
};

// Utility function 
// adds the seconds & nanoseconds time fields of a timespec struct, returns a long long value representing nanoseconds 
long long addspec (struct timespec *ts); 

// Struct to hold more than one arg; used for pthread_create fxn call 
typedef struct add_args{
	long long *ptr; 
	long long iterations; 
	int thread_no; 			// To aid in artifact correction 
} add_args_t; 

// Simple adding routine 
void add(long long *pointer, long long value){
	long long sum = *pointer + value;
	if (opt_yield)
    	pthread_yield(); 
	*pointer = sum; 
}

void mutexAdd(long long *pointer, long long value){
	pthread_mutex_lock(&lock); 
	// critical section
	long long sum = *pointer + value;
	if (opt_yield)
    	pthread_yield(); 
	*pointer = sum; 
	pthread_mutex_unlock(&lock);
}

void spinLockAdd(long long *pointer, long long value){
	while (__sync_lock_test_and_set(&spinlock, 1)) 
		while (spinlock)
			;	// busy wait 
		// critical section
		long long sum = *pointer + value;
		if (opt_yield)
    		pthread_yield(); 
		*pointer = sum;
	__sync_lock_release(&spinlock);
}

void compAndSwapAdd(long long *pointer, long long value){
	long long orig_sum, new_sum, ret; 
	do{
		orig_sum = *pointer; 
		new_sum = orig_sum + value;
		ret = __sync_val_compare_and_swap(pointer, orig_sum, new_sum);
	} while (ret != orig_sum);
	// compare value of orig with current value of *pointer 
}

// Wrapper function for add fxn - used for pthread_create 
void * addSubtractOneNTimes(void * args){
	add_args_t* info= (add_args_t *) args; 
	if(info == NULL)
		return NULL; 
	// Artifact correction 

	void (*add_func)(long long *, long long);	// Function pointer variable 
	switch(sync_option){
		case 'm':
			// use mutex add 
			add_func = mutexAdd; 
			break;
		case 's': 
			// use spin lock add 
			add_func = spinLockAdd; 
			break;
		case 'c':
			add_func = compAndSwapAdd;
			break; 
		default:
			// use default(unsynchronized) add
			add_func = add; 
	}
	// Add 1 and then subtract 1, for N info->iterations times
	// Artifact correction, if --correct asserted 
	if (correct_flag){
		if(clock_gettime(CLOCK_MONOTONIC, &(thread_create_start[info->thread_no]))){
				fprintf(stderr, "clock_gettime failed.");
				n_errors++;
		}
		if (addspec(&(thread_create_start[info->thread_no])) < smallest_start_time)
			smallest_start_time = addspec(&(thread_create_start[info->thread_no]));
	}
	int c;
	for(c = 0; c < info->iterations; c++){
		add_func(info->ptr, 1); 
		add_func(info->ptr, -1); 
	}
	if (correct_flag){
		if(clock_gettime(CLOCK_MONOTONIC, &(thread_create_end[info->thread_no]))){
				fprintf(stderr, "clock_gettime failed.");
				n_errors++;
		}
		if (addspec(&(thread_create_end[info->thread_no])) > largest_end_time)
			largest_end_time = addspec(&(thread_create_end[info->thread_no]));
	}
	return NULL;
}

long long addspec (struct timespec *ts){
	// Check if ts is nullptr here 
	long long sec = (long long) ts->tv_sec; 
	long long nsec = (long long) ts->tv_nsec;
	return (sec * pow(10, 9) + nsec); 
}

int main(int argc, char *argv[]){
	// Default # of threads and iterations is 1 
	int n_threads = 1; 
	int n_iterations = 1; 

	// timer variables
	struct timespec t_start; 
	struct timespec t_end; 

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
			case 'y': 
				if (optarg)
					opt_yield = atoi(optarg);
				break;
			case 's': 
				if (optarg)
					sync_option = optarg[0]; // only the first letter is significant 
				break;
			case 0: 
				// getopt_long set a flag
				break;
			case '?':
			default:
				n_errors++;
		} 
	}

	// Error checking 
	if(n_threads < 1)
		fprintf(stderr, "Number of threads cannot be negative or zero\n"); 
	if(n_iterations < 1)
		fprintf(stderr, "Number of iterations cannot be negative or zero\n"); 

	// Mutex initialization
	if (pthread_mutex_init(&lock, NULL) != 0){
		printf("Mutex initialization failed\n"); 
		n_errors++; 
	}

	pthread_t tids[n_threads]; 			// Array to hold thread ids 
	add_args_t targs_arr[n_threads]; 	// Array to hold thread structs with args to pass 
	thread_create_start = (struct timespec *) malloc (sizeof(struct timespec) * n_threads); 	// Array for artifact correction 
	thread_create_end = (struct timespec *) malloc (sizeof(struct timespec) * n_threads);
	int i, retVal;
		
	// Record start time with high resolution 
	if (clock_gettime(CLOCK_MONOTONIC, &t_start)){ 
		fprintf(stderr, "clock_gettime failed.");
		n_errors++;
	}

	// Create n_threads threads and call the addSubtractOneNTimes function (see above)
	for(i = 0; i < n_threads; i++){
		targs_arr[i].ptr = &count;
		targs_arr[i].iterations = n_iterations; 
		targs_arr[i].thread_no = i; 

		retVal = pthread_create(&tids[i], NULL, addSubtractOneNTimes, &targs_arr[i]); 
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

	// Record start time with high resolution 
	if(clock_gettime(CLOCK_MONOTONIC, &t_end)){
		fprintf(stderr, "clock_gettime failed.");
		n_errors++;
	}

	int n_operations = n_threads * n_iterations * 2;
	printf("%d threads x %d iterations/thread x 2 ops/iteration = %d operations\n", n_threads, n_iterations, n_operations); 
	if(count){
		fprintf(stderr, "ERROR: Final count value = %lld.\n", count); 
		n_errors++;
	}

	// Calculate total elapsed time in threads and output
	long long total_elapsed_time = addspec(&t_end) - addspec(&t_start); 

	// Artifact correction 
	if (correct_flag)
		total_elapsed_time = largest_end_time - smallest_start_time; 
	printf("Elapsed time: %lld ns\n", total_elapsed_time); 
	printf("Average time per operation: %lld ns\n", total_elapsed_time/n_operations);
	
	pthread_mutex_destroy(&lock); 
	free(thread_create_start);
	free(thread_create_end);
	// Free timespecs

	return n_errors; 
}
