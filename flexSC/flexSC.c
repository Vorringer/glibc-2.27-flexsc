#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "flexSC.h"


extern volatile void *_syscall_page;

extern int FLEXSC_REGISTERED;

const char *syscall_file = "/dev/shmem_dev";
int INNER_REGISTERED = 0;



//_syscall_page = NULL;
//FLEXSC_REGISTERED = 0;



long write_syscall(long args[], unsigned int syscall_num) {
	int index = -1;
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
			index = i;
		}
	}
	return wait_syscall(index);
}

long wait_syscall(volatile int index) {
	if (index == -1) return -1;
	Syscall_entry *syscall_page = (Syscall_entry *)_syscall_page;
	//struct timeval start, end;
	//gettimeofday(&start, NULL);
	while (syscall_page[index].status != SYSCALL_ENTRY_DONE) asm("");

	long ret = (unsigned long)syscall_page[index].ret_value;

	syscall_page[index].status = SYSCALL_ENTRY_FREE;
	return ret;
}

long
syscall_noflexsc(long *args, unsigned int sysnum) {
    return syscall6(sysnum,
                    args[0], args[1], args[2],
                    args[3], args[4], args[5]);
}


//flexSC_syscall_t __flexsc_syscall_handle = syscall_noflexsc;

int flexSC_register(void) {
	if (INNER_REGISTERED) return 0;
	int fd;
	int page_size;
	page_size = sysconf(_SC_PAGE_SIZE);
	fd = open(syscall_file, O_RDWR | O_SYNC);
    if (fd < 0) {
    	printf("open device error!\n");
        return -1;
    }
    _syscall_page = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!_syscall_page) {
    	printf("mmap syscall page error!\n");
    	return -1;
    }

    __flexsc_syscall_handle = write_syscall;

    syscall(332);

    FLEXSC_REGISTERED = 1;
    INNER_REGISTERED = 1;


	return 0;
}



int test_write(void) {
    //TODO
    return 0;
}



