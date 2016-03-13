#ifndef WEENSYOS_SCHEDOS_APP_H
#define WEENSYOS_SCHEDOS_APP_H
#include "schedos.h"
#include "schedos-kern.h"

/*****************************************************************************
 * schedos-app.h
 *
 *   System call functions and constants used by SchedOS applications.
 *
 *****************************************************************************/


// The number of times each application should run
#define RUNCOUNT	320


/*****************************************************************************
 * sys_yield
 *
 *   Yield control of the CPU to the kernel, which will pick another
 *   process to run.  (It might run this process again, depending on the
 *   scheduling policy.)
 *
 *****************************************************************************/

static inline void
sys_yield(void)
{
	// We call a system call by causing an interrupt with the 'int'
	// instruction.  In weensyos, the type of system call is indicated
	// by the interrupt number -- here, INT_SYS_YIELD.
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_YIELD)
		     : "cc", "memory");
}


/*****************************************************************************
 * sys_exit(status)
 *
 *   Exit the current process with exit status 'status'.
 *
 *****************************************************************************/

static inline void sys_exit(int status) __attribute__ ((noreturn));
static inline void
sys_exit(int status)
{
	// Different system call, different interrupt number (INT_SYS_EXIT).
	// This time, we also pass an argument to the system call.
	// We do this by loading the argument into a known register; then
	// the kernel can look up that register value to read the argument.
	// Here, the status is loaded into register %eax.
	// You can load other registers with similar syntax; specifically:
	//	"a" = %eax, "b" = %ebx, "c" = %ecx, "d" = %edx,
	//	"S" = %esi, "D" = %edi.
	asm volatile("int %0\n"
		     : : "i" (INT_SYS_EXIT),
		         "a" (status)
		     : "cc", "memory");
    loop: goto loop; // Convince GCC that function truly does not return.
}

/*****************************************************************************
 * sys_priority(???)
 *
 *   IF YOU IMPLEMENT EXERCISE 4.A, NAME YOUR SYSTEM CALL sys_priority .
 *
 *****************************************************************************/

static inline void sys_priority(unsigned priority){
	asm volatile("int %0\n"
			 : : "i" (INT_SYS_PRIORITY),
			 	 "a" (priority)
			 : "cc", "memory");
}


/*****************************************************************************
 * sys_share(???)
 *
 *   IF YOU IMPLEMENT EXERCISE 4.B, NAME YOUR SYSTEM CALL sys_share .
 *
 *****************************************************************************/

static inline void sys_share(unsigned priority){
	asm volatile("int %0\n"
			 : : "i" (INT_SYS_PROPORTIONAL_SHARE),
			 	 "a" (priority)
			 : "cc", "memory");
}
/*****************************************************************************
* sys_print_char(unsigned char)
*
* Hands over control to interrupt handler. The character to be printed is placed 
in %eax. The interrupt number is INT_SYS_PRIORITY 
*
******************************************************************************/
static inline void sys_print_char(unsigned character){
	asm volatile("int %0\n"
			 : : "i" (INT_SYS_PRINT_CHAR),
			 	 "a" (character)
			 : "cc", "memory"); 
}

#endif


