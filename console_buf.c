#include "console_buf.h"
#include "syscall.h"
#include "kos.h"

int console_buffer[CONSOLE_BUFSIZ];

kt_sem nelem;
kt_sem nslots;
kt_sem consoleWait;
int cb_head = 0;
int cb_tail = 0;


void* console_buf() {
    // initialize the nelem, nslots, and consoleWait semaphores
    nelem = make_kt_sem(0);
    // nslots semaphore should be initialized to the number of slots in the circular buffer.
    nslots = make_kt_sem(CONSOLE_BUFSIZ);
    
    consoleWait = make_kt_sem(0);


    while(1) {
        // Call P() on consoleWait.  It will unblock when a character is ready to read from the console. 
        P_kt_sem(consoleWait);

        // Call P() on nslots. If the buffer is full, it will block until the doRead() call drains it a bit.
        P_kt_sem(nslots);

        // Because the P() on nslots is passed it knows that there is space in the buffer.
        console_buffer[cb_head] = (int)console_read();

        // increment the head variable
        // if head variable is greater than or equal to 256, set it back to zero
        cb_head = (cb_head + 1) % 256;

        V_kt_sem(nelem);
    }




}