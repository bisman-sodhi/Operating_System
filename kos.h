#ifndef KOS_H
#define KOS_H

#include "dllist.h"
#include "jrb.h"
#include "simulator.h"
#include "kt.h"

#define STDIN          0
#define STDOUT         1
#define STDERR         2

#define FD_READ        3
#define FD_WRITE       4

#define FD_CONSOLE     5
#define FD_PIPE        6

#define PIPE_BUFF_SIZE 8192
#define NUM_OF_FD      256

struct Pipe{
    unsigned char* pipe_buffer;
    
    // count
    int number_of_writers;
    int number_of_readers;
    
    int start;
    int end;
    int size;
    bool no_more_writes;    
    // semaphores
    kt_sem writers_sem_pipe;
    kt_sem readers_sem_pipe;
    kt_sem available_sem_pipe;
    kt_sem empty_sem_pipe; 
};

struct file_descriptor {
    int fd_id;              // std in std out std err
    int console_pipe_type;  // FD_CONSOLE or FD_PIPE  . 
    int read_write_access;  // read write access FD_READ or FD_WRITE . pipe[0] pipe[1]
    struct Pipe *pipe;      // pipes
};


typedef struct PCB_stc PCB; 
struct PCB_stc {
    
    int registers[NumTotalRegs];
    // The srbk() pointer should be a part of the PCB
    int sbrk_ptr;
    // add base and limit int
    int my_base;
    int my_limit;
    unsigned short my_pid;
    // add pointer to the parent PCB
    PCB* my_parent_PCB;
    // "Add a semaphore called waiter_sem
    // and a dllist called waiters to each process's PCB."
    kt_sem waiter_sem;
    Dllist waiters;
    JRB children;
    int my_exit;
    struct file_descriptor *my_fd[NUM_OF_FD]; // Piazza post 233 size has to bigger than 32



};

void* initialize_user_process(void *arg);
int perform_execve(PCB* job, char* fn, char** argv);

extern Dllist readyq;

extern PCB* curr_PCB;
extern PCB* sentinel_PCB;

extern JRB pid_tree;
extern unsigned short curr_pid;

int get_new_pid();
void destroy_pid(int pid);
void init_fd(PCB* pcb_arg); 

//semaphore to check if okay to write
extern kt_sem write_okay_sem;
extern kt_sem writers_sem;
extern kt_sem readers_sem;
void KOS();


#endif