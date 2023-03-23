#include <stdlib.h>
#include "kos.h"
#include "simulator.h"
#include "kt.h"
#include "dllist.h"
#include "scheduler.h"
#include "console_buf.h"
#include "memory.h"


Dllist readyq;
PCB* curr_PCB;
PCB* sentinel_PCB;

kt_sem write_okay_sem;
kt_sem writers_sem;
kt_sem readers_sem;


JRB pid_tree;
unsigned short curr_pid = 0;

Mem_Segs* mem_seg_ptr;


int perform_execve(PCB* job, char* fn, char** argv){
	// perform_execve() loads the user program, sets the stack and the registers, and then returns zero.
	int load_result = load_user_program(argv[0]);
	if (load_result < 0) {
		fprintf(stderr,"Can't load program.\n");
		return -1;
	}
	else{
		job->sbrk_ptr = load_result;
	}
	// job init
	int i;
	for (i=0; i < NumTotalRegs; i++) {
	 	job->registers[i] = 0;
	}
	job->registers[PCReg] = 0;
	job->registers[NextPCReg] = 4;
	job->my_base = User_Base;
	job->my_limit = User_Limit;
	job->registers[StackReg] = job->my_limit - 12;

	InitUserArgs(job->registers, argv, job->my_base);
	return 0; 
}

void init_fd(PCB* pcb_arg){
	int i = 0;
	for (i; i < NUM_OF_FD; i++){
		pcb_arg->my_fd[i] = malloc(sizeof(struct file_descriptor));
		if (i == 0){
			pcb_arg->my_fd[i]->fd_id = STDIN;
			pcb_arg->my_fd[i]->console_pipe_type = FD_CONSOLE;
			pcb_arg->my_fd[i]->pipe = NULL;
			pcb_arg->my_fd[i]->read_write_access = -1;
		}
		else if( i == 1){
			pcb_arg->my_fd[i]->fd_id = STDOUT;
			pcb_arg->my_fd[i]->console_pipe_type = FD_CONSOLE;
			pcb_arg->my_fd[i]->pipe = NULL;
			pcb_arg->my_fd[i]->read_write_access = -1;
		}
		else if( i == 2){
			pcb_arg->my_fd[i]->fd_id = STDERR;
			pcb_arg->my_fd[i]->console_pipe_type = FD_CONSOLE;
			pcb_arg->my_fd[i]->pipe = NULL;
			pcb_arg->my_fd[i]->read_write_access = -1;
		}
		else{
			pcb_arg->my_fd[i]->fd_id = -1;
			pcb_arg->my_fd[i]->console_pipe_type = FD_PIPE;
			pcb_arg->my_fd[i]->read_write_access = -1;
			pcb_arg->my_fd[i]->pipe = NULL;
		}
	}
}

void* initialize_user_process(void *arg) {
	// split initialize_user_process into 2 parts
	// init everything
	// set the User_base and User_limit registers
	User_Base = 0;
	User_Limit = MemorySize / 8;
	char **my_argv = (char **)arg;
	// create a sentinel PCB
	sentinel_PCB = (PCB *)malloc(sizeof(PCB));
	// assign a pid of 0 to the sentinel PCB
	sentinel_PCB->my_pid = 0;
	sentinel_PCB->waiter_sem = make_kt_sem(0);
	sentinel_PCB->waiters = new_dllist();
	sentinel_PCB->children = make_jrb();

	// Allocate a new PCB
	curr_PCB = (PCB *)malloc(sizeof(PCB));
	// "initialize the pid of your first process in initialize_user_process()"
	curr_PCB->my_pid = get_new_pid();
	// make sentinel PCB the parent of the first process
	curr_PCB->my_parent_PCB = sentinel_PCB;

	init_fd(curr_PCB);

	curr_PCB->children = make_jrb();
	jrb_insert_int(sentinel_PCB->children, curr_PCB->my_pid, new_jval_v((void * ) curr_PCB));
	// "Initialize the waiter semaphore to zero."
	curr_PCB->waiter_sem = make_kt_sem(0);
	// "Initialize the waiters list"
	curr_PCB->waiters = new_dllist();

	int execve_result = (int)perform_execve(curr_PCB, my_argv[0], my_argv);
	
	// put the PCB at the end of the ready queue
	if(execve_result == 0) {
		// need to mark memory segment 0 as "in use"
		mem_seg_ptr->mem_seg[0] = TRUE;
		dll_append(readyq, new_jval_v((void *)curr_PCB));
		kt_exit();
	}
	else {
		exit(1);
	}
}

int get_new_pid() {
	JRB cursor;
	Jval pid_payload;
	// initialize curr_pid to 0 (need to do this everytime if you want the lowest possible pid val)
	curr_pid = 0;
	// increment curr_pid
	curr_pid++;
	bool pid_avail = FALSE;
	while (!pid_avail) {
		// look for curr_pid in the tree
		cursor = jrb_find_int(pid_tree, curr_pid);
		if (cursor == NULL) { 
			// means that curr_pid is available
			pid_avail = TRUE;
		}
		else {
			// increment curr_pid and continue the while loop
			curr_pid++;
		}
	}
	// once available pid is found, we need to add it to the tree
	pid_payload = new_jval_i(curr_pid);
	jrb_insert_int(pid_tree, curr_pid, pid_payload);
	return curr_pid;
}

void destroy_pid(int pid) {
	JRB cursor;
	cursor = jrb_find_int(pid_tree, pid);
	jrb_delete_node(cursor);

}

void KOS() {

	bzero(main_memory, MemorySize);

	readyq = new_dllist();
	if(readyq == NULL) {
		exit(1);
	}

	/*
	 * initialize an empty r-b tree
	 */
	pid_tree = make_jrb();
	if(pid_tree == NULL) {
		exit(1);
	}

	// initialize the semaphores
	write_okay_sem = make_kt_sem(0);
	writers_sem = make_kt_sem(1);
	readers_sem = make_kt_sem(1);

	// initialize array for tracking memory segments
	mem_seg_ptr = (Mem_Segs*)malloc(sizeof(Mem_Segs));
	memset(mem_seg_ptr->mem_seg, FALSE, 8 * sizeof(bool));

	kt_fork(initialize_user_process, (void*)kos_argv);
	kt_fork(console_buf, (void*)kos_argv[0]);
	start_timer(10);
	scheduler();

	/* not reached .. */
}