// Simpleton Shell
// CS111 Winter 2016
// Eggert 
// Lab 1C

// Note: errors do not cause simpleton shell to exit. The shell tries its best to 
// finish parsing all options. A diagnostic message is sent to stderr if an action 
// could not be finished. 

#include <unistd.h> 	
#include <fcntl.h> 	
#include <errno.h> 
#include <getopt.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <ctype.h>
#include <string.h> 
#include <sys/time.h> 
#include <sys/resource.h> 
#include <math.h> 

#define TABLE_SIZE 100 
// Boilerplate code 

// Flags
int verbose_flag = 0; 
int append_flag = 0; 
int cloexec_flag = 0; 
int creat_flag = 0; 
int directory_flag = 0; 
int dsync_flag = 0; 
int excl_flag = 0; 
int nofollow_flag = 0; 
int nonblock_flag = 0; 
int rsync_flag = 0; 
int sync_flag = 0; 
int trunc_flag = 0; 

// Sub-command option flag 
int profile_flag = 0; 
int wait_flag = 0; 
// Table variables 
int table_size = TABLE_SIZE; 
int table_index = 0; 
// Fix this up later 
// static char* usage = "usage: ./simpsh [OPTION]... [STREAM]... [FILE]..."; 

// getopt_long signature 
/* int getopt_long(int argc, char * const argv[],
           const char *optstring,
           const struct option *longopts, int *longindex);
*/ 
// Recall program name is argv[0]

static struct option long_options[] = {
	/*********************************************
	These options do not set a flag.
	*********************************************/
	// File-opening options 
	{"rdonly",	required_argument, 0, 'r'}, 
	{"wronly", required_argument, 0, 'w'}, 
	{"rdwr", required_argument, 0, 'v'}, 
	{"pipe", no_argument, 0, 'p'}, 
	// Sub-command options 
	{"command", required_argument, 0, 'c'}, 
	{"wait", no_argument, &wait_flag, '1'},
	// Miscellaneous options
	{"close", required_argument, 0, 'e'}, 
	{"abort", no_argument, 0, 'a'}, 
	{"catch", required_argument, 0, 'h'}, 
	{"ignore", required_argument, 0, 'i'}, 
	{"default", required_argument, 0, 'd'}, 
	{"pause", no_argument, 0, 'u'},
	/**********************************************
	These options do set a flag. 
	**********************************************/
	// File-opening flags & verbose, profile option
	{"profile", no_argument, &profile_flag, 1}, 
	{"verbose", no_argument, &verbose_flag, 1},
	{"append", no_argument, &append_flag, O_APPEND},
	{"cloexec", no_argument, &cloexec_flag, O_CLOEXEC},
	{"creat", no_argument, &creat_flag, O_CREAT},
	{"directory", no_argument, &directory_flag, O_DIRECTORY},
	{"dsync", no_argument, &dsync_flag, O_DSYNC},
	{"excl", no_argument, &excl_flag, O_EXCL},
	{"nofollow", no_argument, &nofollow_flag, O_NOFOLLOW},
	{"nonblock", no_argument, &nonblock_flag, O_NONBLOCK},
	{"rsync", no_argument, &rsync_flag, O_RSYNC},
	{"sync", no_argument, &sync_flag, O_SYNC},
	{"trunc", no_argument, &trunc_flag, O_TRUNC},
	{0, 0, 0, 0}
	};

// Takes no args 
void reset_file_flags(void){
	// Monitor file-opening flags here & reset them
	append_flag = 0; cloexec_flag = 0; creat_flag = 0; directory_flag = 0; dsync_flag = 0; 
	excl_flag = 0; nofollow_flag = 0; nonblock_flag = 0; rsync_flag = 0; sync_flag = 0; trunc_flag = 0; 
}

int combined_file_flags(void){
	// OR all available file-opening flags 
	return (append_flag | cloexec_flag | creat_flag | directory_flag | dsync_flag | excl_flag | nofollow_flag | nonblock_flag | rsync_flag | sync_flag | trunc_flag); 
}

void sighandler(int signum){ 
	fprintf(stderr, "Caught signal %d. Exiting simpsh.\n", signum); 
	exit(signum); 
}

double addtv (struct timeval t){
	double d_sec = (double) t.tv_sec; 
	double d_usec = (double) t.tv_usec; 
	return (d_sec + (d_usec * pow(10, -6))); 
}
	
typedef struct pidinfo {
	pid_t c_pid; 
	char* cmd_argv[10];
} pidinfo_t;
	

int main(int argc, char** argv){
	/*********************
	Housekeeping variables 
	**********************/ 
	// File descriptor table 
	int* file_desc_table = (int*) malloc(table_size * sizeof(int)); 	// Stores logical file descriptors 
	if (!file_desc_table) {
		fprintf(stderr, "Memory allocation for file descriptor table failed. Exiting Simpleton Shell."); 
		exit(1); 
	}
	int desc_is_pipe[100]; 	// Stores info about whether fd is one end of a pipe 
	// Variables for read, write, readwrite file-opening options 
	int fd; 
	char* filename; 
	int command_failed = 0; 
	
	// Variables to parse the --command option with 
	int in_fd, out_fd, err_fd; 		// stores the in, out, and err arguments
	char* cmd;	// stores the cmd and its arg
	char* cmd_args[10];
	int arg_cnt; // keep track of how many args in --command 
	pidinfo_t child_pa[1000]; 
	int pa_index = 0; // start off with 0 child processes 
	int max_exit_status = 0; // in case program exits in unusual way, exit with this status
	
	// Variables to keep track of where we are in getopt_long
	int optchar; // stores value returned by getopt_long
	int long_index = 0; // stores index of current option in long_options array 
	struct option current_longopt;
	struct rusage current; 
	/*********************
	Parse command-line args 
	**********************/ 
	while(1){ 
		opterr = 0; // Silence getopt errors 
		// Before parsing, check for valid optind -- we should be done if this happens 
		if (optind >= argc)
			break; 
		// printf("Current optind is: %d\n", optind);
		optchar = getopt_long(argc, argv, ":", long_options, &long_index);  
		// printf("Current optchar is: %c\n", optchar); 
		/* If no more options, escape */ 
		if (optchar == -1)
			break; 
		
		// printf("Current optind is: %d\n", optind); 
		if(verbose_flag){
			// verbose flag is set 
			// Output command along with operands 
			// Because --command has multiple arguments, output it specifically 
			// in the switch statement block
			current_longopt = long_options[long_index]; 
			// Options that require an argument, except --command 
			int not_command = strcmp(current_longopt.name, "command"); // will return non-zero value if it is not command  
			if (current_longopt.has_arg == required_argument && not_command){
				printf("--%s ", current_longopt.name); 
				printf("%s\n", optarg); 
			}
			
			// Options that do not require an argument go here 
			int not_verbose = strcmp(current_longopt.name, "verbose");  // will return non-zero value if it is not verbose 
			if (current_longopt.has_arg == no_argument && not_verbose)
				printf("--%s\n", current_longopt.name); 
		}
		
		// If necessary, make file desc table bigger
		if (table_index == TABLE_SIZE){
			table_size *= 2; 
			int* a = (int*) realloc(file_desc_table, table_size * sizeof(int)); 
			if(!a){
				fprintf(stderr, "Reallocation for file descriptor table failed. Exiting.\n"); 
				exit(1);
			}
		}
		
		arg_cnt = 0; 
		switch(optchar){
			case 0: 
				// getopt_long() set a flag 
				break; 
			case 'r': 
				// open a file for reading; store real fd into descriptor table 
				filename = optarg; 
				fd = open(filename, O_RDONLY | combined_file_flags(), 0644); 
				// open only successful if fd returned is not -1 
				if(fd < 0){
					// failed to open file 
					perror(filename);
					exit(max_exit_status); 
					command_failed = 1;  
				}
				if(fd >= 0){
					file_desc_table[table_index] = fd;
					desc_is_pipe[table_index] = 0; 
					table_index++;
				}
				// clear the file option flags for the next file, if any 
				reset_file_flags(); 
				break; 
				// Check for errors 
			case 'w': 
				// open a file for reading; store real fd into descriptor table 
				filename = optarg; 
				fd = open(filename, O_WRONLY | combined_file_flags(), 0644); 
				if(fd < 0){
					// failed to open file 
					perror(filename);
					exit(max_exit_status);
					command_failed = 1;  
				}
				if(fd >= 0){
					file_desc_table[table_index] = fd;
					desc_is_pipe[table_index] = 0; 
					table_index++;
				}
				// clear the file option flags for the next file, if any 
				reset_file_flags(); 
				break; 
				// Check for errors 
			case 'v':
				// open a file for reading & writing; store real fd into descriptor table
				filename = optarg; 
				fd = open(filename, O_RDWR | combined_file_flags(), 0644); 
				if(fd < 0){
					// failed to open file 
					perror(filename);
					exit(max_exit_status); 
					command_failed = 1;  
				}
				if(fd >= 0){
					file_desc_table[table_index] = fd;
					desc_is_pipe[table_index] = 0; 
					table_index++;
				}
				// clear the file option flags for the next file, if any 
				reset_file_flags(); 
				break; 
			case 'c':{
				// execute a shell command 
				// the first 3 command-line args should be input, output, error streams
				// that refer to logical file descriptors 
				
				// input fd 
				if (optind < argc){
					in_fd = atoi(argv[optind-1]);
					if (in_fd >= table_index){
						fprintf(stderr, "Invalid standard input file descriptor specified. Continuing.\n");
						// exit(1); 
					}
			
					// printf("%d\n", in_fd); 
				}
				// output fd 
				if (optind < argc){
					out_fd = atoi(argv[optind++]); 
					if (out_fd >= table_index){
						fprintf(stderr, "Invalid standard output file descriptor specified. Continuing.\n");
						// exit(1); 
					}
					// printf("%d\n", out_fd); 
				}
				// error fd 
				if (optind < argc){
					err_fd = atoi(argv[optind++]);
					if (err_fd >= table_index){
						fprintf(stderr, "Invalid standard error file descriptor specified. Continuing.\n");
						// exit(1); 
					}
					// printf("%d\n", err_fd); 
				}
				// cmd to be executed 
				if (optind < argc){
					cmd = argv[optind++]; 
					cmd_args[arg_cnt] = cmd; 
					arg_cnt++; 
					// printf("%s\n", cmd); 
				}
				
				// Parsing command-line args 
				int offset = 0; 
				while(1){
					if(optind < argc){
						int temp = optind;
						int len = strlen(argv[optind]); 
						optchar = getopt_long(argc, argv, ":", long_options, &long_index);
						if(isalpha(optchar)){ 
							// still another option to be parsed
							offset = optind - temp;
							optind = temp; 
							break; 
						}
						else if (optchar == 0){
							// We hit a long option that set a flag, exit this loop 
							offset = optind - temp;
							optind = temp; 
							break;
						}
						
						else if (optchar == '?'){
							if (len < 3){
								cmd_args[arg_cnt] = argv[optind-1];  
								arg_cnt++;
							}
							else{
								cmd_args[arg_cnt] = argv[optind];  
								arg_cnt++;
								break; 
							}
						}
						else{
							// 
							optind = temp; 
							cmd_args[arg_cnt] = argv[optind++];  
							arg_cnt++;
						}
					}
					// no arguments left to parse
					else 
						// optind is greater than/equal to argc 
						break; 
				}
				
				// This portion of code handles commands one after another 
				int next_pos;
				// Handle extra commands 
				current_longopt = long_options[long_index]; 
				if (current_longopt.has_arg == no_argument)
					next_pos = optind + offset - 1;
				if (current_longopt.has_arg == required_argument)
					next_pos = optind + offset - 2; 
				for(; optind < next_pos; optind++)
					cmd_args[arg_cnt++] = argv[optind];
					
				// cmd_arg array passed to execvp must be terminated by NULL 
				cmd_args[arg_cnt] = NULL;
				
				// Handle verbose flag, if set
				if (verbose_flag){
					fprintf(stdout, "--command %d %d %d ", in_fd, out_fd, err_fd); 
					int j;
					for(j=0; j<arg_cnt; j++)
					// Not sure why this works
						fprintf(stdout, "%s ", cmd_args[j]); 
					printf("\n"); 
				}	
				
				// Check if this command is dealing with pipes.. If it is, we need to deal with it 
				// Assign fds given to --command to existing in, out, err streams (0, 1, 2) 
			
				pid_t pid = fork(); 
				if(pid == 0){ 
					dup2(file_desc_table[in_fd], 0); 
					dup2(file_desc_table[out_fd], 1); 
					dup2(file_desc_table[err_fd], 2); 
					// Close unused ends of pipes 
					/*
					printf("%d\n", desc_is_pipe[in_fd]); 
					if(desc_is_pipe[in_fd] == 1){
						close(file_desc_table[in_fd]); 
						close(file_desc_table[in_fd + 1]); 
					}
					if(desc_is_pipe[out_fd]){
						close(file_desc_table[out_fd]); 
						close(file_desc_table[out_fd - 1]); 
					}
					*/
					// Close all descriptors 
					int i; 
					for(i=0; i < table_index; i++){
						// Possibly closing file descriptors twice
						close(file_desc_table[i]); 
					}
					if(execvp(cmd, cmd_args)){ 
						perror("execvp: "); 
						// exit(1); 
					}
				}
				else if(pid > 0){
					// parent process
					if(desc_is_pipe[in_fd])
						close(file_desc_table[in_fd]); 
					if(desc_is_pipe[out_fd])
						close(file_desc_table[out_fd]); 
					
					// Store info about child process into child_pa array 
					child_pa[pa_index].c_pid = pid; 
					int j; 
					for(j=0; j<arg_cnt; j++)
						child_pa[pa_index].cmd_argv[j] = cmd_args[j];
					pa_index++; 
				}
				else {
					// fork failed
					perror("fork: "); 
					exit(1); 
				}
					
				break; 	
			}
			case 'e':{
				// Close a file descriptor
				// Once closed, it is an error to access file N. 
				// File numbers are not reused in ./simpsh 
				int file_no = atoi(optarg); // Convert char* to int 
				if (file_no < 0){
					fprintf(stderr, "%d is an invalid and negative file descriptor. Continuing\n.", file_no); 
					command_failed = 1; 
					break; 
					// exit(1); 
				}
				if (file_no >= table_index){
					fprintf(stderr, "No file descriptor exists with number %d. Continuing\n.", file_no); 
					command_failed = 1; 
					break; 
					// exit(1);
				}
				close(file_desc_table[file_no]); 
				break;
			}
			case 'p':{
				int pipefd[2]; 
				// Create a pipe 
				// Need to fork 
				if(pipe(pipefd) == -1){
					perror("pipe"); 
					command_failed = 1;  
					break; 
				}
				// printf("Pipe created two file descriptors, %d and %d.\n", pipefd[0], pipefd[1]); 
				file_desc_table[table_index] = pipefd[0]; // read end
				desc_is_pipe[table_index++] = 1; 
				file_desc_table[table_index] = pipefd[1]; // write end
				desc_is_pipe[table_index++] = 1; 
				
				// Update the pipe table as well 
				/* 
				pid_t pid = fork(); 
				if (pid == 0){
					// fork a child process, with its stdin replaced with pipefd[0] 
					close(STDIN_FILENO); 
					dup(pipefd[0]); 
					close(pipefd[0]); 
					close(pipefd[1]); 
				}
				else if (pid > 0){
					// parent process
					close(pipefd[0]);
					close(pipefd[1]); 
					int childstatus; 
					wait(pid, &childstatus, 0); // blocking wait
				}
				else
					perror("Fork failed:"); */ 
				break;
			}
			case 'a': 
				// crash the shell via seg fault 
				raise(SIGSEGV);
				break;
			case 'u': 
				// pause, waiting for external signal 
				pause(); 
				break; 
			case 'i':{
				// Ignore signal with sig no. N 
				int sig_no = atoi(optarg); 
				signal(sig_no, SIG_IGN); 
				break; 
			}
			case 'd':{
				// Use default behavior for sig no. N 
				int sig_no = atoi(optarg); 
				signal(sig_no, SIG_DFL); 
				break;
			}   
			case 'h':{
				int sig_no = atoi(optarg); 
				signal(sig_no, sighandler); // handle sig_no manually 
				break; 
			}
			case ':': 
				/* missing option argument; but if : is not first character of optstring, 
				this case is lumped into '?' case */ 
				fprintf(stderr, "Option is missing operand(s). Continuing.\n");  
				break;
			case '?':{
				perror("Invalid option: "); 
				printf("%s\n", argv[optind]); 
				break; 
			}
			default: 
				/* invalid option specified */ 
				// perror("Invalid option: "); 
				// fprintf(stderr, "%s: option '%s' is invalid. Continuing.\n", argv[0], argv[optind-1]); 
				break; 
		}
		
			if(profile_flag){
				if(!getrusage(RUSAGE_SELF, &current)){
				// addtv()
				double usr_time = addtv(current.ru_utime); 
				double sys_time = addtv(current.ru_stime);
				printf("User time: %fs	System time: %fs\n", usr_time, sys_time); 
				}
				else
				perror("rsuage: ");
			}
	}
	
	// Testing 
	// Close file descriptors here (good practice)
	// Free allocated memory
	
	int i; 
	for(i=0; i < table_index; i++){
		// Possibly closing file descriptors twice
		close(file_desc_table[i]); 
	}
	free(file_desc_table); 
	
	// Check for wait-flag here. If set, wait for remaining commands to finish and output exit status of child followed by cmd & cmd_args 
	if(wait_flag){
		int i; 
		for(i=0; i<pa_index; i++){
			int childStatus; 
			int child_pid = waitpid(-1, &childStatus, 0); // return immediately if no child has yet exited 
			int exitStatus = WEXITSTATUS(childStatus); 
			if (exitStatus > max_exit_status) 
				max_exit_status = exitStatus; 
			if (child_pid == -1)
				perror("wait error: "); 
			printf("%d ", exitStatus); 
			int j; 
			for(j=0; j<pa_index; j++){
				if(child_pa[j].c_pid == child_pid){
					int k=0;
					while(child_pa[j].cmd_argv[k] != NULL){
						printf("%s ", child_pa[j].cmd_argv[k]);
						k++; 
					}
					break; 
				}
			}
			printf("\n"); 
		}
	}
	
		if(profile_flag){
				if(!getrusage(RUSAGE_CHILDREN, &current)){
				// addtv()
				double usr_time = addtv(current.ru_utime); 
				double sys_time = addtv(current.ru_stime);
				printf("User time: %fs	System time: %fs\n", usr_time, sys_time); 
				}
				else
				perror("rsuage: ");
			}
	
	// Program exits with maximum exit status of one of its subcommands if it did not execute properly 
	if(pa_index && max_exit_status) 
		exit(max_exit_status); 
	// If no subcommands, or if all commands performed correctly, check if any command options failed
	if(command_failed)
		exit(1); 
	
	// All commands & subcommands performed correctly, return normally 
	return 0; 
}

