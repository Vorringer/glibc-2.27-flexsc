#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "flexSC.h"


extern void *_syscall_page;

extern int FLEXSC_REGISTERED;
extern const char *syscall_file;

long hehehe = 1;

_syscall_page = NULL;
FLEXSC_REGISTERED = 0;



int write_syscall(long args[], unsigned int syscall_num) {
	Syscall_entry *syscall_page = (Syscall_entry *)_syscall_page;
	for (int i = 0; i < MAX_ENTRY; ++i) {
		if (syscall_page[i].status == SYSCALL_ENTRY_FREE) {
			syscall_page[i].status = SYSCALL_ENTRY_BLOCKED;
			syscall_page[i].syscall_num = syscall_num;
			syscall_page[i].arg0 = args[0];
			syscall_page[i].arg1 = args[1];
			syscall_page[i].arg2 = args[2];
			syscall_page[i].arg3 = args[3];
			syscall_page[i].arg4 = args[4];
			syscall_page[i].arg5 = args[5];
			syscall_page[i].status = SYSCALL_ENTRY_SUBMITED;
			return i;
		}
	}
	return -1;
}

unsigned long wait_syscall(int index) {
	if (index == -1) return -1;
	Syscall_entry *syscall_page = (Syscall_entry *)_syscall_page;
	//struct timeval start, end;
	//gettimeofday(&start, NULL);
	while (1) {
		if (syscall_page[index].status == SYSCALL_ENTRY_DONE) {
			return (unsigned long)syscall_page[index].ret_value;
		}
		//gettimeofday(&end, NULL);
		//if (1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec > 5000) break;
	}
	return -1;
}

int flexSC_register(void) {
	if (FLEXSC_REGISTERED) return 0;
	int fd;
	int page_size;
	page_size = sysconf(_SC_PAGE_SIZE);
	fd = open(syscall_file, O_RDWR | O_SYNC);
    if (fd < 0) {
        return -1;
    }
    _syscall_page = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (!_syscall_page) {
    	return -1;
    }


    syscall(332);

    FLEXSC_REGISTERED = 1;


	return 0;
}


int test_write(void) {
    //TODO
    return 0;
}



