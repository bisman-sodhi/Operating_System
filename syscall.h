#ifndef SYSCALL_H
#define SYSCALL_H

#include "kos.h"

void* do_write(void* PCB_arg);

void* do_read(void* PCB_arg);

void* do_ioctl(void* PCB_arg);

void* do_fstat(void* PCB_arg);

void* do_execve(void* PCB_arg);

void* do_getpagesize(void* PCB_arg);

void syscall_return(PCB* PCB_ptr, int return_val);
 
void* do_sbrk(void* PCB_arg);

void* user_to_kos_address(int address, int base, int limit, int length);

void* do_getpid(void* PCB_arg);

void* do_fork(void* PCB_arg);

void* finish_fork(void* PCB_arg);

void* do_exit(void* PCB_arg);

void* do_getdtablesize(void* PCB_arg);

void* do_close(void* PCB_arg);

void* do_wait(void* PCB_arg);

void* do_getppid(void* PCB_arg);

int find_next_FD(struct file_descriptor ** fd_arg);

void* do_pipe(void* PCB_arg);

void* do_dup(void* PCB_arg);

void* do_dup2(void* PCB_arg);


#endif