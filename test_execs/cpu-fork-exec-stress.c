#include <stdio.h>
main()
{
    int i, count;
    int pid;

    pid = getpid();

    for (count = i = 0; i < 5; i++) {
	count += i; 
	printf("pid: %d, i: %d\n",pid,i);
    }
    exit(0);
}
