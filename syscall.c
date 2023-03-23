

#include <stdlib.h>
#include "kos.h"
#include "simulator.h"
#include "dllist.h"
#include "kt.h"
#include "scheduler.h"
#include "errno.h"
#include "console_buf.h"
#include "memory.h"



// lab2 step5 helper function
void* user_to_kos_address(int address, int base, int limit, int length) {
    if((address < 0) || (address + length > limit)) {
        return NULL;
    }
    return &main_memory[address + base];
}

/** 
 * 
 *  1 - Set PCReg in the saved registers to NextPCReg.  If you don't
 *      do this, the process will keep calling the system call.  
 *  2 - Put the return value into register 2.
 *  3 - Put the PCB onto the ready queue.
 *  4 - Call kt_exit
 * 
 */

void syscall_return(PCB* PCB_ptr, int return_val) {    
    PCB_ptr->registers[PCReg] = PCB_ptr->registers[NextPCReg];
    PCB_ptr->registers[2] = return_val;
    dll_append(readyq, new_jval_v((void*)PCB_ptr));
    kt_exit();
}

void* do_read(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (struct PCB_stc*)(PCB_arg);

    char* buf_iter = user_to_kos_address(local_PCB->registers[6], local_PCB->my_base, local_PCB->my_limit, local_PCB->registers[7]);
    if (buf_iter == NULL) {
        syscall_return(local_PCB, -EFAULT);
    }
    if (local_PCB->registers[5] == 1 || local_PCB->registers[5] == 2){
        syscall_return(local_PCB, -EBADF);
    }
    if (local_PCB->my_fd[local_PCB->registers[5]]->fd_id == -1){
        syscall_return(local_PCB, -EBADF);
    }
    if (local_PCB->registers[6] < 0){
        syscall_return(local_PCB, -EFAULT);
    }
    if (local_PCB->registers[7] < 0){
        syscall_return(local_PCB, -EINVAL);
    }
    int mem_using = local_PCB->registers[5] + local_PCB->registers[6];
    if (mem_using > MemorySize){
        syscall_return(local_PCB, -ENOMEM);
    }

    if(local_PCB->my_fd[local_PCB->registers[5]]->console_pipe_type == FD_CONSOLE){
    int return_char = 0;
    char* readAddress = main_memory + local_PCB->registers[6];

    while (return_char < local_PCB->registers[7] ) {
        P_kt_sem(nelem);

        // -1 indicate EOF
        if (console_buffer[cb_tail] == -1) {

            // increment the tail variable
            // if tail variable is greater than or equal to 256, set it back to zero
            cb_tail = (cb_tail + 1) % 256;
            // The doRead() call will need to call V() on nslots
	        // every time it remove a character from the buffer so that the
	        // semaphore counts the number of available slots correctly.
            V_kt_sem(nslots);
            break;
        }
        readAddress[return_char] = console_buffer[cb_tail];
        // increment the tail variable
        // if tail variable is greater than or equal to 256, set it back to zero
        cb_tail = (cb_tail + 1) % 256;
        return_char++;
        V_kt_sem(nslots);

        
    }

    syscall_return(local_PCB, return_char);
    }
    if(local_PCB->my_fd[local_PCB->registers[5]]->console_pipe_type == FD_PIPE){

        int return_char = 0;
        char* readAddress = main_memory + local_PCB->registers[6] + local_PCB->my_base;
        
        if(local_PCB->my_fd[local_PCB->registers[5]]->read_write_access == FD_READ){
            P_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->readers_sem_pipe);               
            while(return_char < local_PCB->registers[7]){
                if(kt_getval(local_PCB->my_fd[local_PCB->registers[5]]->pipe->empty_sem_pipe) == 0){
                    V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->readers_sem_pipe);
                    syscall_return(local_PCB, return_char);
                }
                if(local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_writers == 0 ){
                    V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->readers_sem_pipe);
                    syscall_return(local_PCB, 0);
                }
                if(local_PCB->my_fd[local_PCB->registers[5]]->pipe->no_more_writes == TRUE){
                    V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->readers_sem_pipe);
                    break;
                }
                P_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->empty_sem_pipe);
                readAddress[return_char] = (char)local_PCB->my_fd[local_PCB->registers[5]]->pipe->pipe_buffer[local_PCB->my_fd[local_PCB->registers[5]]->pipe->start];
                local_PCB->my_fd[local_PCB->registers[5]]->pipe->start = (local_PCB->my_fd[local_PCB->registers[5]]->pipe->start + 1) % local_PCB->my_fd[local_PCB->registers[5]]->pipe->size;
                V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->available_sem_pipe);
                return_char++;
        
            }
        }
        else{
            syscall_return(local_PCB, -EBADF);
        }
        V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->readers_sem_pipe);
        syscall_return(local_PCB, return_char);
    }    

}

void* do_write(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    char* buf_iter = user_to_kos_address(local_PCB->registers[6], local_PCB->my_base, local_PCB->my_limit, local_PCB->registers[7]);

    if (buf_iter == NULL) {
        syscall_return(local_PCB, -EFAULT);
    }
    if (local_PCB->my_fd[local_PCB->registers[5]]->fd_id == -1){
        syscall_return(local_PCB, -EBADF);
    }
    if ( (local_PCB->registers[6] < 0) || (local_PCB->registers[6]%4 !=0)){
        syscall_return(local_PCB, -EFAULT);
    }
    if (local_PCB->registers[7] < 0){
        syscall_return(local_PCB, -EINVAL);
    }
    int mem_using = local_PCB->registers[5] + local_PCB->registers[6];
    if (mem_using > MemorySize){
        syscall_return(local_PCB, -EFBIG);
    }

    if(local_PCB->my_fd[local_PCB->registers[5]]->fd_id == STDOUT || local_PCB->my_fd[local_PCB->registers[5]]->fd_id == STDERR){
    int i;
    P_kt_sem(writers_sem);
    for (i = 0; i < local_PCB->registers[7]; i++){
        console_write(buf_iter[i]);
        P_kt_sem(write_okay_sem);
    }
    V_kt_sem(writers_sem);

    syscall_return(local_PCB, i);
    }
    if(local_PCB->my_fd[local_PCB->registers[5]]->console_pipe_type == FD_PIPE){
        char* readAddress = main_memory + local_PCB->registers[6];
        if(local_PCB->my_fd[local_PCB->registers[5]]->read_write_access == FD_WRITE){
            
        if ((local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_readers) == 0)
        {
            syscall_return(local_PCB, -EPIPE);
        }

        P_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->writers_sem_pipe);
        int i = 0;
        while( i < local_PCB->registers[7]){              
            P_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->available_sem_pipe);
            if(local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_readers == 0){
                syscall_return(local_PCB, i);
            }
            local_PCB->my_fd[local_PCB->registers[5]]->pipe->pipe_buffer[local_PCB->my_fd[local_PCB->registers[5]]->pipe->end] = readAddress[i]; //buf_iter[i];
            if((local_PCB->my_fd[local_PCB->registers[5]]->pipe->end + 1) % local_PCB->my_fd[local_PCB->registers[5]]->pipe->size != local_PCB->my_fd[local_PCB->registers[5]]->pipe->start){
                local_PCB->my_fd[local_PCB->registers[5]]->pipe->end = (local_PCB->my_fd[local_PCB->registers[5]]->pipe->end + 1) % local_PCB->my_fd[local_PCB->registers[5]]->pipe->size;
            }

            if((local_PCB->my_fd[local_PCB->registers[5]]->pipe->end + 1) % local_PCB->my_fd[local_PCB->registers[5]]->pipe->size == local_PCB->my_fd[local_PCB->registers[5]]->pipe->start){
                local_PCB->my_fd[local_PCB->registers[5]]->pipe->no_more_writes = TRUE;
            }


            V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->empty_sem_pipe);
   
            i++;
        }
        V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->writers_sem_pipe);
        syscall_return(local_PCB, i);
        }
        else{
            syscall_return(local_PCB, -EBADF);
        }
        if(local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_readers == 0){
            syscall_return(local_PCB, -EPIPE);
        }
    }    

}

void* do_ioctl(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    struct JOStermios* jos_termios_ptr = user_to_kos_address(local_PCB->registers[7], local_PCB->my_base, local_PCB->my_limit, sizeof(struct JOStermios));
    if (jos_termios_ptr == NULL){
        syscall_return(local_PCB, -EFAULT);
    }
    if((local_PCB->registers[5] != 1) || (local_PCB->registers[6] != JOS_TCGETP)) {
        syscall_return(local_PCB, -EINVAL);
    }
    ioctl_console_fill(jos_termios_ptr);
    syscall_return(local_PCB, 0);
}
/**
 * "You will service these system calls by calling stat_buf_fill(char *addr, int blk_size),
 * where addr is the KOS address of the stat struct that the user passed to fstat(), 
 * and blk_size is the amount of buffering that the file descriptor should allow. 
 * For file descriptor zero, this should be one. For file descriptors one and two, try 256." 
 */
void* do_fstat(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    int buff_size;

    if(local_PCB->registers[5] == 0) {
        buff_size = 1;
    }
    else if (local_PCB->registers[5] == 1 || local_PCB->registers[5] == 2) {
        buff_size = 256;
    }

    char* buf_ptr = user_to_kos_address(local_PCB->registers[6], local_PCB->my_base, local_PCB->my_limit, buff_size);
    if (buf_ptr == NULL){
        syscall_return(local_PCB, -EFAULT);
    }
   
    stat_buf_fill((struct KOSstat *)buf_ptr, buff_size);
    syscall_return(local_PCB, 0);

}

void do_getpagesize(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    syscall_return(local_PCB, PageSize);
}


void* do_sbrk(void* PCB_arg){
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);


    char* user_address = user_to_kos_address(local_PCB->sbrk_ptr, local_PCB->my_base, local_PCB->my_limit, local_PCB->registers[5]);
    
    if (user_address == NULL) {
        syscall_return(local_PCB, -EFAULT);
    }

    else {
        int curr_sbrk = local_PCB->sbrk_ptr;
        local_PCB->sbrk_ptr += local_PCB->registers[5];
        syscall_return(local_PCB, curr_sbrk);
        
    }
}


void* do_execve(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);

    // fn 
    int fn_len = strlen((char *)(&main_memory[local_PCB->registers[5] + local_PCB->my_base]));
    char * fn = (char * ) malloc(sizeof (char) * fn_len );
    memcpy(fn, &main_memory[local_PCB->registers[5] + local_PCB->my_base] , fn_len + 1);

    // argv
    char ** argv = (char **) &main_memory[local_PCB->registers[6] + local_PCB->my_base];
    int number_of_arguments = 0;
	if(argv != NULL){
		while( argv[number_of_arguments] != NULL){
			number_of_arguments++;
	    }
    }
    // malloc enough space for all arguments and memcpy arguments into this space
    char ** argv_list = (char **)malloc(sizeof (char *) * (number_of_arguments + 1));
    int i, arg_offset; 
    char * arg;
    for(i = 0; i < number_of_arguments; i++){  
            arg_offset = (int)argv[i];
            arg = &main_memory[local_PCB->my_base + arg_offset];
            argv_list[i] = (char *) malloc(sizeof(char) * (strlen(arg) + 1));
            memcpy(argv_list[i], arg, (strlen(arg) + 1));     
    }
    argv_list[number_of_arguments] = NULL;
    for(i = 0; i < number_of_arguments; i++){
    }

    if( perform_execve(local_PCB, fn, argv_list) == 0){
        free(fn);
        free(argv_list); 

        local_PCB->registers[2] = 0;
        dll_append(readyq, new_jval_v((void*)local_PCB));
        kt_exit();

    }
    else{
        free(fn);
        free(argv_list);
        local_PCB->registers[2] = -1;
        dll_append(readyq, new_jval_v((void*)local_PCB));
        kt_exit();
    }
}



void* do_getpid(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    unsigned short local_pid;
    local_pid = local_PCB->my_pid;
    syscall_return(local_PCB, (int) local_pid);
}

void* finish_fork(void* PCB_arg) {
    PCB* newjob;
    newjob = (PCB*)(PCB_arg);
    syscall_return(newjob, 0);
}

// Beginning incremental implementation of fork() system call
void* do_fork(void* PCB_arg) {
    PCB* local_PCB;
    PCB* child_PCB;

    int seg_size;
    local_PCB = (PCB*)(PCB_arg);
    seg_size = MemorySize / 8;
    // check to see if you have room in memory, and if not, return EAGAIN.
    int child_mem_seg;
    child_mem_seg = find_available_mem_seg(mem_seg_ptr);
    if(child_mem_seg == -1) {
        syscall_return(local_PCB, -EAGAIN);
    }
    else {
        // If it's ok, allocate a new PCB and initialize its fields --
        // limit and base will be to a new part of memory.
        child_PCB = (PCB *)malloc(sizeof(PCB));
        // Lab 2, Step 18: assign the my_parent_PCB pointer of the child to local PCB pointer
        child_PCB->my_parent_PCB = local_PCB;
        // Step 21
        child_PCB->children = make_jrb();
        // set the new base
        child_PCB->my_base = seg_size * child_mem_seg;
        child_PCB->sbrk_ptr = local_PCB->sbrk_ptr;
        // set the new limit
        //IMPORTANT: LIMIT IS THE SIZE OF MEMORY SEGMENT, NOT THE LOCATION OF THE END OF MEMORY SEGMENT
        child_PCB->my_limit = seg_size;
        // The registers should be copied from the calling process's PCB
        memcpy(child_PCB->registers, local_PCB->registers, sizeof(int) * NumTotalRegs);
        // It should get a new pid. 
        child_PCB->my_pid = get_new_pid();
        // Waiters dll initialization and waiter_sem semaphore initialization
        // "Initialize the waiter semaphore to zero."
	    child_PCB->waiter_sem = make_kt_sem(0);
	    // "Initialize the waiters list"
	    child_PCB->waiters = new_dllist();
        // Its memory should be a copy of the calling process's memory.
        memcpy(&main_memory[child_PCB->my_base], &main_memory[local_PCB->my_base], seg_size);
        int i = 0;
        for(i; i < 48; i++){
            if(local_PCB->my_fd[i] != NULL){
                child_PCB->my_fd[i] = ( struct file_descriptor *)malloc(sizeof(struct file_descriptor *));
                child_PCB->my_fd[i]->fd_id             = local_PCB->my_fd[i]->fd_id;
                child_PCB->my_fd[i]->console_pipe_type = local_PCB->my_fd[i]->console_pipe_type;
                if( local_PCB->my_fd[i]->console_pipe_type == FD_PIPE){
                    child_PCB->my_fd[i]->pipe              = local_PCB->my_fd[i]->pipe;
                    child_PCB->my_fd[i]->read_write_access = local_PCB->my_fd[i]->read_write_access;
                    if(local_PCB->my_fd[i]->read_write_access == FD_READ){
                        local_PCB->my_fd[i]->pipe->number_of_readers++;
                    }
                    if(local_PCB->my_fd[i]->read_write_access == FD_WRITE){
                        local_PCB->my_fd[i]->pipe->number_of_writers++;
                    }
                }
            }
        }

        jrb_insert_int(local_PCB->children, child_PCB->my_pid, new_jval_v((void * )child_PCB ));
        kt_fork(finish_fork, (void*)child_PCB);
        syscall_return(local_PCB, child_PCB->my_pid);
    }    
}

void* do_exit(void* PCB_arg) {
    PCB* local_PCB;
    
    local_PCB = (PCB*)(PCB_arg);
    int seg_size;
    int mem_segment_num;
    seg_size = MemorySize / 8;
    mem_segment_num = local_PCB->my_base / seg_size;
    mem_seg_ptr->mem_seg[mem_segment_num] = FALSE;

    int i = 0;
    for(i; i < 48; i++){
        if(local_PCB->my_fd[i] != NULL){
            if( local_PCB->my_fd[i]->console_pipe_type == FD_PIPE ){
                if(local_PCB->my_fd[i]->read_write_access == FD_WRITE){
                    local_PCB->my_fd[i]->pipe->number_of_writers--;
                    if (local_PCB->my_fd[i]->pipe->number_of_writers <= 0){
                        local_PCB->my_fd[i]->pipe->no_more_writes = TRUE;
                    }
                    if(local_PCB->my_fd[i]->pipe->number_of_writers == 0){
                    V_kt_sem(local_PCB->my_fd[i]->pipe->empty_sem_pipe);
                    }
                }
                if(local_PCB->my_fd[i]->read_write_access == FD_READ){
                    local_PCB->my_fd[i]->pipe->number_of_readers--;
                    if(local_PCB->my_fd[i]->pipe->number_of_readers == 0){
                    V_kt_sem(local_PCB->my_fd[i]->pipe->available_sem_pipe);
                    }
                }
            }
        }

    }

    PCB* temp;
    JRB orphan_child;
    Jval payload;
    // when a PCB process dies, it needs to switch parentage of its children to sentinel_PCB
    while (!(jrb_empty(local_PCB->children))) {
        // grab first value from children jrb and get the PCB
        temp = (PCB *)jval_v(jrb_val(jrb_first(local_PCB->children)));
        // change the parent of temp to sentinel_PCB
        temp->my_parent_PCB = sentinel_PCB;
        // insert temp into the children tree of sentinel
        jrb_insert_int(sentinel_PCB->children, temp->my_pid, new_jval_v((void*)temp));
        // delete the child off local_PCB's children tree
        jrb_delete_node(jrb_find_int(local_PCB->children, temp->my_pid));
    }

    jrb_delete_node(jrb_find_int(local_PCB->my_parent_PCB->children, local_PCB->my_pid));
    local_PCB->my_exit = local_PCB->registers[5];

    Dllist zombie_cursor;
    while (!(dll_empty(local_PCB->waiters))) {
        zombie_cursor = dll_first(local_PCB->waiters);

        temp = (PCB *)jval_v(dll_val(zombie_cursor));
        free(temp);
        dll_delete_node(zombie_cursor);
    }
    if(local_PCB->my_parent_PCB->my_pid == 0){
  
        free(local_PCB);
    }
    else{
        // When a process exits, it should call V() on its parent's waiter_sem,
        // and put its PCB onto the parents waiters list.
        dll_append(local_PCB->my_parent_PCB->waiters, new_jval_v((void *)local_PCB));
        V_kt_sem(local_PCB->my_parent_PCB->waiter_sem);
    }
    /**
     * TESTING: print all the values in the sentinel PCB's child list
     * 
     */
    kt_exit();
}

void* do_getdtablesize(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    syscall_return(local_PCB, 64);
}

// "Implement the close() system call so that it returns -EBADF 
// whenever it is called.  We're going to ignore the close() system call until 
// the next lab, but we have to deal with the calls that ksh will make."
void* do_close(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    if(local_PCB->my_fd[local_PCB->registers[5]] == NULL){
        syscall_return(local_PCB, -EBADF);
    }
    if(local_PCB->registers[5] > 48 || local_PCB->registers[5] < 0){
        syscall_return(local_PCB, -EBADF); 
    }

    if( local_PCB->my_fd[local_PCB->registers[5]]->console_pipe_type == FD_PIPE ){
        if(local_PCB->my_fd[local_PCB->registers[5]]->read_write_access == FD_WRITE){
            local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_writers--;
            if (local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_writers <= 0){
                local_PCB->my_fd[local_PCB->registers[5]]->pipe->no_more_writes = TRUE;
            }
            if(local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_writers == 0){
               V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->empty_sem_pipe);
            }
        }
        if(local_PCB->my_fd[local_PCB->registers[5]]->read_write_access == FD_READ){
            local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_readers--;
            if(local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_readers == 0){
               V_kt_sem(local_PCB->my_fd[local_PCB->registers[5]]->pipe->available_sem_pipe);
            }
        }

    }
    syscall_return(local_PCB, 0);

}

void* do_getppid(void* PCB_arg) {
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    unsigned short parent_pid;
    parent_pid = local_PCB->my_parent_PCB->my_pid;
    syscall_return(local_PCB, (int) parent_pid);
}

void* do_wait(void* PCB_arg) {
    PCB* local_PCB;
    PCB* zombie_child_PCB;
    unsigned short child_pid;
    local_PCB = (PCB*)(PCB_arg);
    if(dll_empty(local_PCB->waiters)){
        // The calling process does not have any unwaited-for children.
        syscall_return(local_PCB, -ECHILD);
    }
    if(jrb_empty(local_PCB->children) && dll_empty(local_PCB->waiters)) {
        syscall_return(local_PCB, -1);
    }
    // "Now, when a process calls wait(), it should call P() on its waiter_sem semaphore."
    P_kt_sem(local_PCB->waiter_sem);
    /*
    If only one child process is terminated, then return a wait() returns process ID of the terminated child process. 
    If more than one child processes are terminated than wait() reap any arbitrarily child and return a process ID of that child process. 
    */
    // copy the child off that waiters dllist to zombie_child_PCB pointer
    zombie_child_PCB = (PCB *)(jval_v(dll_val(dll_first(local_PCB->waiters))));
    child_pid = zombie_child_PCB->my_pid;
    memcpy(&(main_memory[local_PCB->registers[5]+ local_PCB->my_base]),&(zombie_child_PCB->my_exit), 4);
    // delete the zombie child from the waiters dllist
    dll_delete_node(dll_first(local_PCB->waiters)); // dll_first ?
    // free the child pid
    destroy_pid((int) zombie_child_PCB->my_pid);
    // free the child PCB
    free_dllist(zombie_child_PCB->waiters);
    jrb_free_tree(zombie_child_PCB->children);
    free(zombie_child_PCB);
    syscall_return(local_PCB, (int)child_pid);

}

int find_next_FD(struct file_descriptor ** fd_arg) {

    int i = 0;
    for (i; i < 48; i++){
        if(fd_arg[i]->fd_id == -1){
            fd_arg[i]->fd_id = i;
            return i;
        }
    }
    return -1;
}

void* do_pipe(void* PCB_arg) {    
    //  * pipefd[0] refers to the read end of the pipe.  
    //  * pipefd[1] refersto the write end of the pipe.
    
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    struct Pipe* local_pipe = malloc(sizeof(struct Pipe));
     
    local_pipe->pipe_buffer = malloc(sizeof(unsigned char)* PIPE_BUFF_SIZE);
    local_pipe->start = 0;
    local_pipe->end = 0;
    local_pipe->size = PIPE_BUFF_SIZE;
    local_pipe->no_more_writes = FALSE;
    local_pipe->writers_sem_pipe = make_kt_sem(1);
    local_pipe->readers_sem_pipe = make_kt_sem(1);
    local_pipe->available_sem_pipe = make_kt_sem(PIPE_BUFF_SIZE);
    local_pipe->empty_sem_pipe = make_kt_sem(0);
    local_pipe->number_of_writers = 1;
    local_pipe->number_of_readers = 1;

    int next_read_fd = find_next_FD(local_PCB->my_fd);
    int next_write_fd = find_next_FD(local_PCB->my_fd);

    local_PCB->my_fd[next_read_fd]->fd_id = next_read_fd;
    local_PCB->my_fd[next_read_fd]->console_pipe_type = FD_PIPE;
    local_PCB->my_fd[next_read_fd]->read_write_access = FD_READ;

    local_PCB->my_fd[next_write_fd]->fd_id = next_write_fd;
    local_PCB->my_fd[next_write_fd]->console_pipe_type = FD_PIPE;
    local_PCB->my_fd[next_write_fd]->read_write_access = FD_WRITE;

    if(local_PCB->my_fd[next_read_fd]->read_write_access == FD_READ){
        local_pipe->number_of_readers++;
    }
    if(local_PCB->my_fd[next_write_fd]->read_write_access == FD_WRITE){
        local_pipe->number_of_writers++;
    }

    local_PCB->my_fd[next_read_fd]->pipe = local_pipe;
    local_PCB->my_fd[next_write_fd]->pipe = local_pipe;

    memcpy(&(main_memory[local_PCB->registers[5] + local_PCB->my_base]), &(next_read_fd), sizeof(next_read_fd));
    memcpy(&(main_memory[local_PCB->registers[5] + local_PCB->my_base + 4]), &(next_write_fd), sizeof(next_write_fd));
    
    syscall_return(local_PCB, 0);
}

void* do_dup(void* PCB_arg){
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    int orig_fd = local_PCB->registers[5];
    int dup_fd_id = find_next_FD(local_PCB->my_fd);
    struct file_descriptor * dup_fd =  local_PCB->my_fd[dup_fd_id];

    if ( orig_fd < 0 || orig_fd > 48){
        syscall_return(local_PCB, -EBADF);
    }
    if ( dup_fd_id > 48){
		syscall_return(local_PCB, -EMFILE);
    }
    if( local_PCB->my_fd[local_PCB->registers[5]] == NULL){
        syscall_return(local_PCB, -EBADF);
    }
    
    if( dup_fd_id > -1){
        dup_fd->fd_id = dup_fd_id;
        dup_fd->console_pipe_type = local_PCB->my_fd[local_PCB->registers[5]]->console_pipe_type;
        dup_fd->read_write_access = local_PCB->my_fd[local_PCB->registers[5]]->read_write_access;
        if (dup_fd->read_write_access == FD_READ){
            local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_readers++;
        }
        if(dup_fd->read_write_access == FD_WRITE){
            local_PCB->my_fd[local_PCB->registers[5]]->pipe->number_of_writers++;
        }
        dup_fd->pipe = local_PCB->my_fd[local_PCB->registers[5]]->pipe;
        syscall_return(local_PCB, dup_fd_id);
    }

}

void* do_dup2(void* PCB_arg){
    PCB* local_PCB;
    local_PCB = (PCB*)(PCB_arg);
    int dup2_id = local_PCB->registers[6];
    int orig_id = local_PCB->registers[5];
    
    if (local_PCB->my_fd[orig_id] == NULL){
        syscall_return(local_PCB, -EBADF);
    }
    
    if( 0 > dup2_id || 0 > orig_id){
        syscall_return(local_PCB, -EBADF);
    }
    
    if ( orig_id > 48 || dup2_id > 48){
        syscall_return(local_PCB, -EBADF);
    }

    if( local_PCB->my_fd[dup2_id] == local_PCB->my_fd[orig_id]){
        syscall_return(local_PCB, dup2_id);
    }

    local_PCB->my_fd[dup2_id]->fd_id = local_PCB->my_fd[orig_id]->fd_id;
    local_PCB->my_fd[dup2_id]->console_pipe_type = local_PCB->my_fd[orig_id]->console_pipe_type;
    local_PCB->my_fd[dup2_id]->read_write_access = local_PCB->my_fd[orig_id]->read_write_access;
    local_PCB->my_fd[dup2_id]->pipe = local_PCB->my_fd[orig_id]->pipe;
    
    if(local_PCB->my_fd[dup2_id]->read_write_access == FD_WRITE){
        local_PCB->my_fd[orig_id]->pipe->number_of_writers++;
    }
    
    if(local_PCB->my_fd[dup2_id]->read_write_access == FD_READ){
        local_PCB->my_fd[orig_id]->pipe->number_of_readers++;
    }
    syscall_return(local_PCB, dup2_id);
}