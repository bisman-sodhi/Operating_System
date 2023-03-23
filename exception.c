/*
 * exception.c -- stub to handle user mode exceptions, including system calls
 * 
 * Everything else core dumps.
 * 
 * Copyright (c) 1992 The Regents of the University of California. All rights
 * reserved.  See copyright.h for copyright notice and limitation of
 * liability and disclaimer of warranty provisions.
 */
#include <stdlib.h>

#include "simulator.h"
#include "kos.h"
#include "syscall.h"
#include "scheduler.h"
#include "dllist.h"
#include "kt.h"
#include "console_buf.h"

void
exceptionHandler(ExceptionType which)
{
	int             type, r5, r6, r7, newPC;
	int             buf[NumTotalRegs];

	examine_registers(buf);
	// system call type is in register 4
	type = buf[4];
	r5 = buf[5];
	r6 = buf[6];
	r7 = buf[7];
	newPC = buf[NextPCReg];

	// when you first get into exceptionHandler(), you should save the
	// registers into your PCB struct
	
	memcpy(curr_PCB->registers, buf, sizeof(buf));


	/*
	 * for system calls type is in r4, arg1 is in r5, arg2 is in r6, and
	 * arg3 is in r7 put result in r2 and don't forget to increment the
	 * pc before returning!
	 */

	switch (which) {
	case SyscallException:
		/* the numbers for system calls is in <sys/syscall.h> */
		switch (type) {
		case 0:
			/* 0 is our halt system call number */
			DEBUG('e', "Halt initiated by user program\n");
			SYSHalt();

		case SYS_exit:
			/* this is the _exit() system call */
			DEBUG('e', "_exit() system call\n");
			kt_fork(do_exit, (void*) curr_PCB);
			break; 

		case SYS_write:
			DEBUG('e', "_write() system call\n");
			kt_fork(do_write, (void*) curr_PCB);
			break;

		case SYS_read:
			DEBUG('e', "_read() system call\n");
			kt_fork(do_read, (void*) curr_PCB);
			break;

		// the ioctl system call implementation
		case SYS_ioctl:
			DEBUG('e', "_ioctl() system call\n");
			kt_fork(do_ioctl, (void*) curr_PCB);
			break;

		// the fstat system call implementation
		case SYS_fstat:
			DEBUG('e', "_fstat() system call\n");
			kt_fork(do_fstat, (void*) curr_PCB);
			break;

		// getpagesize() system call implementation
		case SYS_getpagesize:
			DEBUG('e', "_getpagesize() system call\n");
			kt_fork(do_getpagesize, (void*) curr_PCB);
			break;

		case SYS_sbrk:
			DEBUG('e', "_sbrk() system call\n");
			kt_fork(do_sbrk, (void*)curr_PCB);
			break;

		case SYS_execve:
			DEBUG('e', "_execve() system call\n");
			kt_fork(do_execve, (void*)curr_PCB);
			break;

	    // getpid() system call implementation
		case SYS_getpid:
			DEBUG('e', "_getpid() system call\n");
			kt_fork(do_getpid, (void*)curr_PCB);
			break;

		// fork() system call implementation
		case SYS_fork:
			DEBUG('e', "_fork() system call\n");
			kt_fork(do_fork, (void*)curr_PCB);
			break;

		// getdtablesize() system call implementation
		case SYS_getdtablesize:
			DEBUG('e', "_getdtablesize() system call\n");
			kt_fork(do_getdtablesize, (void*)curr_PCB);
			break;

		// close() system call implementation
		case SYS_close:
			DEBUG('e', "_close() system call\n");
			kt_fork(do_close, (void*)curr_PCB);
			break;

		// wait() system call implementation
		case SYS_wait:
			DEBUG('e', "_wait() system call\n");
			kt_fork(do_wait, (void*)curr_PCB);
			break;

		// getppid() system call implementation
		case SYS_getppid:
			DEBUG('e', "_wait() system call\n");
			kt_fork(do_getppid, (void*)curr_PCB);
			break;

		case SYS_pipe:
			DEBUG('e', "_pipe() system call\n");
			kt_fork(do_pipe, (void*)curr_PCB);
			break;

		case SYS_dup:
			DEBUG('e', "_dup() system call\n");
			kt_fork(do_dup, (void*)curr_PCB);
			break;

		case SYS_dup2:
			DEBUG('e', "_dup2() system call\n");
			kt_fork(do_dup2, (void*)curr_PCB);
			break;	

		default:
			DEBUG('e', "Unknown syem call\n");
			SYSHalt();
			break;
		}
		break;
	case PageFaultException:
		DEBUG('e', "Exception PageFaultException\n");
		break;
	case BusErrorException:
		DEBUG('e', "Exception BusErrorException\n");
		break;
	case AddressErrorException:
		DEBUG('e', "Exception AddressErrorException\n");
		break;
	case OverflowException:
		DEBUG('e', "Exception OverflowException\n");
		break;
	case IllegalInstrException:
		DEBUG('e', "Exception IllegalInstrException\n");
		break;
	default:
		printf("Unexpected user mode exception %d %d\n", which, type);
		exit(1);
	}
	scheduler();
}


void
interruptHandler(IntType which) {
	/**
	 * save the state of the program and put it back onto the readyq before
        processing the interrupt
	 */
	
	if (curr_PCB != NULL) {
		examine_registers(curr_PCB->registers);
		dll_append(readyq, new_jval_v((void *)curr_PCB));
	}


	switch (which) {
	case ConsoleReadInt:
		DEBUG('e', "ConsoleReadInt interrupt\n");
		// when processing the ConsoleReadInt interrupt, call V() on consoleWait
		V_kt_sem(consoleWait);
		break;
	case ConsoleWriteInt:
		DEBUG('e', "ConsoleWriteInt interrupt\n");
		// have the ConsoleWriteInt case call V(writeok)
		V_kt_sem(write_okay_sem);
		break;
	case TimerInt:
		DEBUG('e', "TimerInt interrupt\n");
		break;
	default:
		DEBUG('e', "Unknown interrupt\n");
		break;
	}
	scheduler();
}