#ifndef _FLEXSC_H_
#define _FLEXSC_H_


typedef struct {
	int syscall_num;
	short aug_num;
	short status;
	long ret_value;
	long arg0;
	long arg1;
	long arg2;
	long arg3;
	long arg4;
	long arg5;
} Syscall_entry;





#define MAX_ENTRY (64)

#define SYSCALL_ENTRY_FREE (0)
#define SYSCALL_ENTRY_SUBMITED (1)
#define SYSCALL_ENTRY_DONE (2)
#define SYSCALL_ENTRY_BLOCKED (3)

#endif