#include "schedos-kern.h"
#include "x86.h"
#include "lib.h"

/*****************************************************************************
 * schedos-kern
 *
 *   This is the schedos's kernel.
 *   It sets up process descriptors for the 4 applications, then runs
 *   them in some schedule.
 *
 *****************************************************************************/

// The program loader loads 4 processes, starting at PROC1_START, allocating
// 1 MB to each process.
// Each process's stack grows down from the top of its memory space.
// (But note that SchedOS processes, like MiniprocOS processes, are not fully
// isolated: any process could modify any part of memory.)

#define NPROCS		5
#define PROC1_START	0x200000
#define PROC_SIZE	0x100000

// +---------+-----------------------+--------+---------------------+---------/
// | Base    | Kernel         Kernel | Shared | App 0         App 0 | App 1
// | Memory  | Code + Data     Stack | Data   | Code + Data   Stack | Code ...
// +---------+-----------------------+--------+---------------------+---------/
// 0x0    0x100000               0x198000 0x200000              0x300000
//
// The program loader puts each application's starting instruction pointer
// at the very top of its stack.
//
// System-wide global variables shared among the kernel and the four
// applications are stored in memory from 0x198000 to 0x200000.  Currently
// there is just one variable there, 'cursorpos', which occupies the four
// bytes of memory 0x198000-0x198003.  You can add more variables by defining
// their addresses in schedos-symbols.ld; make sure they do not overlap!


// A process descriptor for each process.
// Note that proc_array[0] is never used.
// The first application process descriptor is proc_array[1].
static process_t proc_array[NPROCS];

// A pointer to the currently running process.
// This is kept up to date by the run() function, in mpos-x86.c.
process_t *current;

// The preferred scheduling algorithm.
int scheduling_algorithm;

// UNCOMMENT THESE LINES IF YOU DO EXERCISE 4.A
// Use these #defines to initialize your implementation.
// Changing one of these lines should change the initialization.
#define __PRIORITY_1__ 1
#define __PRIORITY_2__ 2
#define __PRIORITY_3__ 3
#define __PRIORITY_4__ 4

// UNCOMMENT THESE LINES IF YOU DO EXERCISE 4.B
// Use these #defines to initialize your implementation.
// Changing one of these lines should change the initialization.
// #define __SHARE_1__ 1
// #define __SHARE_2__ 2
// #define __SHARE_3__ 3
// #define __SHARE_4__ 4

// USE THESE VALUES FOR SETTING THE scheduling_algorithm VARIABLE.
#define FCFS   				0  // the initial algorithm
#define STRICT_PRIORITY   	2  // strict priority scheduling (exercise 2)
#define SET_PRIORITY 		41  // p_priority algorithm (exercise 4.a)
#define PROPORTIONAL_SHARE  42  // p_share algorithm (exercise 4.b)
#define LOTTERY    			7  // any algorithm for exercise 7

#define MAX_PROC_TICKETS 5000 

int tarr[NPROCS * MAX_PROC_TICKETS] = {0}; // Default initialize to zero 
unsigned total_tickets = 0;
/*******************************
Added helper function to figure out if all processes finished 

int allProcessesFinished(){
	pid_t i; 
	for(i=1; i < NPROCS; i++)
	{
		if (proc_array[i].p_state == P_RUNNABLE || proc_array[i].p_state == P_BLOCKED)
			return 0; 
	}
	// all processes are zombies/finished 
	return 1; 
}
********************************/

/******************************************
Added a pseudo-random number generator I found on wikipedia 
It is a XOR Linear FeedBack Shift Register 
******************************************/

// The follwing are randomly assigned values
unsigned x = 0x4253;
unsigned y = 0x8F932;
unsigned z = 0xA900; 
unsigned w = 0x429BB35; 

// Random number generator 
unsigned randNumGen(void){
	unsigned t = x; 
	t ^= t << 11; 
	t ^= t >> 8; 
	x = y; y = z; z = w; 
	w ^= w >> 19;
	w ^= t; 
	return w; 
}

/*****************************************************************************
 * start
 *
 *   Initialize the hardware and process descriptors, then run
 *   the first process.
 *
 *****************************************************************************/

void
start(void)
{
	int i;

	// Set up hardware (schedos-x86.c)
	segments_init();
	interrupt_controller_init(0);
	console_clear();

	// Initialize process descriptors as empty
	memset(proc_array, 0, sizeof(proc_array));
	for (i = 0; i < NPROCS; i++) {
		proc_array[i].p_pid = i;
		proc_array[i].p_state = P_EMPTY;
		proc_array[i].p_priority = 1; 
		proc_array[i].p_share = 1;
		proc_array[i].p_share_count = 0; 
		proc_array[i].p_ticket_count = 0;
	}

	int ticket_index = 0; // To set the ticket's pid 
	// Set up process descriptors (the proc_array[])
	for (i = 1; i < NPROCS; i++) {
		process_t *proc = &proc_array[i];
		uint32_t stack_ptr = PROC1_START + i * PROC_SIZE;

		// Initialize the process descriptor
		special_registers_init(proc);

		// Set ESP
		proc->p_registers.reg_esp = stack_ptr;

		// Load process and set EIP, based on ELF image
		program_loader(i - 1, &proc->p_registers.reg_eip);

		// Mark the process as runnable!
		// And set other fields 
		proc->p_state = P_RUNNABLE;

		// For lottery scheduling 
		proc->p_ticket_count = (randNumGen() % MAX_PROC_TICKETS + 1);	// Assign 1-500 tickets
		total_tickets += proc->p_ticket_count; // update global variable
		int c;
		for(c=0; c < proc->p_ticket_count; c++){
			tarr[ticket_index++] = i; 
		}
	}

	// Initialize the cursor-position shared variable to point to the
	// console's first character (the upper left).
	cursorpos = (uint16_t *) 0xB8000;

	// Initialize the scheduling algorithm.
	// USE THE FOLLOWING VALUES:
	//    0 = the initial algorithm
	//    2 = strict priority scheduling (exercise 2)
	//   41 = p_priority algorithm (exercise 4.a)
	//   42 = p_share algorithm (exercise 4.b)
	//    7 = any algorithm that you may implement for exercise 7
	scheduling_algorithm = 7;

	// Switch to the first process.
	//
	run(&proc_array[1]); 
	// Should never get here!
	while (1)
		/* do nothing */;
}



/*****************************************************************************
 * interrupt
 *
 *   This is the weensy interrupt and system call handler.
 *   The current handler handles 4 different system calls (two of which
 *   do nothing), plus the clock interrupt.
 *
 *   Note that we will never receive clock interrupts while in the kernel.
 *
 *****************************************************************************/

void
interrupt(registers_t *reg)
{
	// Save the current process's register state
	// into its process descriptor
	current->p_registers = *reg;

	switch (reg->reg_intno) {

	case INT_SYS_YIELD:
		// The 'sys_yield' system call asks the kernel to schedule
		// the next process.
		schedule();

	case INT_SYS_EXIT:
		// 'sys_exit' exits the current process: it is marked as
		// non-runnable.
		// The application stored its exit status in the %eax register
		// before calling the system call.  The %eax register has
		// changed by now, but we can read the application's value
		// out of the 'reg' argument.
		// (This shows you how to transfer arguments to system calls!)
		current->p_state = P_ZOMBIE;
		current->p_exit_status = reg->reg_eax;
		schedule();

	case INT_SYS_PRIORITY:
		// 'sys_user*' are provided for your convenience, in case you
		// want to add a system call.
		/* Your code here (if you want). */
		current->p_priority = reg->reg_eax; 
		run(current);

	case INT_SYS_PROPORTIONAL_SHARE:
		/* Your code here (if you want). */
		current->p_share = reg->reg_eax; 
		run(current);

	case INT_CLOCK:
		// A clock interrupt occurred (so an application exhausted its
		// time quantum).
		// Switch to the next runnable process.
		schedule();

	case INT_SYS_PRINT_CHAR:
		// The process wants to print out a character atomically
		*cursorpos++ = reg->reg_eax; 
		run(current);

	default:
		while (1)
			/* do nothing */;

	}
}



/*****************************************************************************
 * schedule
 *
 *   This is the weensy process scheduler.
 *   It picks a runnable process, then context-switches to that process.
 *   If there are no runnable processes, it spins forever.
 *
 *   This function implements multiple scheduling algorithms, depending on
 *   the value of 'scheduling_algorithm'.  We've provided one; in the problem
 *   set you will provide at least one more.
 *
 *****************************************************************************/

void
schedule(void)
{
	pid_t pid = current->p_pid;
	int priority_level = 0x7ffffff;
	int share_count = 0; 
	if (scheduling_algorithm == FCFS)
		while (1) {
			pid = (pid + 1) % NPROCS;

			// Run the selected process, but skip
			// non-runnable processes.
			// Note that the 'run' function does not return.
			if (proc_array[pid].p_state == P_RUNNABLE)
				run(&proc_array[pid]);
		}

	if (scheduling_algorithm == STRICT_PRIORITY){
		// strict priority scheduling 
		// pid == priority in this case
		while(1){
			for(pid = 0; pid < NPROCS; pid++){
				if (proc_array[pid].p_state == P_RUNNABLE)
					run(&proc_array[pid]); 
			}
		}
	}

	if (scheduling_algorithm == SET_PRIORITY){
		int i;
		while(1){
			// there are still processes to be tended to 
			for(i=1; i < NPROCS; i++){
				if( (proc_array[i].p_state == P_RUNNABLE) && (proc_array[i].p_priority < priority_level) )
					priority_level = proc_array[i].p_priority; 
			}

			// increment pid to check if other processes have the same priority 
			pid = (pid + 1) % NPROCS; 
			if( (proc_array[pid].p_state == P_RUNNABLE) && (proc_array[pid].p_priority == priority_level) )
				run(&proc_array[pid]); 
		}
	}

	if (scheduling_algorithm == PROPORTIONAL_SHARE){
		while(1){
			// Use a counter variable to monitor how many times a process has run 
			// Run the processes in Round Robin order + factor in shares
			if(proc_array[pid].p_state == P_RUNNABLE){
				if (proc_array[pid].p_share == proc_array[pid].p_share_count)
					proc_array[pid].p_share_count = 0; 
				else{
					proc_array[pid].p_share_count++; 
					run(&proc_array[pid]); 
				}
			}
			// if process has overrun its share time, go to next process 
			pid = (pid+1) % NPROCS; 
		}
	}
	
	// Problem #7 
	if (scheduling_algorithm == LOTTERY){
		while(1){
			// Pick a random ticket from those generated when kernel booted 
			int rand_ticket = (randNumGen() % total_tickets); 

			// See which process the ticket is assigned to 
			if(proc_array[tarr[rand_ticket]].p_state == P_RUNNABLE)
				run(&proc_array[tarr[rand_ticket]]); 
		}
	}
	// If we get here, we are running an unknown scheduling algorithm.
	cursorpos = console_printf(cursorpos, 0x100, "\nUnknown scheduling algorithm %d\n", scheduling_algorithm);
	while (1)
		/* do nothing */;
}
