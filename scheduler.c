#include <stdlib.h>
#include "scheduler.h"
#include "dllist.h"
#include "kt.h"
#include "kos.h"



void scheduler() {
    kt_joinall();
    if (jrb_empty(sentinel_PCB->children)) {
		SYSHalt();
    }
    if (dll_empty(readyq)) {
        curr_PCB = NULL;
        noop();
    }
    else {
        PCB* pcb_pointer = (PCB *)(jval_v(dll_val(dll_first(readyq))));
        curr_PCB = pcb_pointer; 
        // NEED TO DELETE NODE BEFORE RUN_USER_CODE
        dll_delete_node(dll_first(readyq));
        User_Limit = curr_PCB->my_limit;
        User_Base = curr_PCB->my_base;
        
        run_user_code(pcb_pointer->registers);   
    }    
}
