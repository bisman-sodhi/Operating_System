#ifndef CONSOLE_BUF_H
#define CONSOLE_BUF_H


#define CONSOLE_BUFSIZ 256

#include "kt.h"

extern int console_buffer[CONSOLE_BUFSIZ];
extern int cb_head;
extern int cb_tail;
extern kt_sem nelem;
extern kt_sem nslots;
extern kt_sem consoleWait;

void* console_buf();


#endif