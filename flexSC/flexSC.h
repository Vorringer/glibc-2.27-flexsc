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

typedef long (*flexSC_syscall_t)(long *sysargs, unsigned int sysnum);
extern int INNER_REGISTERED;
extern const char *syscall_file;

#define __syscall_XX(sysnum, arg0, arg1, arg2, arg3, arg4, arg5) ({     \
            int __sysnum = (int)(sysnum);                               \
            register long __arg0 asm ("rdi") = (long)(arg0);        \
            register long __arg1 asm ("rsi") = (long)(arg1);        \
            register long __arg2 asm ("rdx") = (long)(arg2);        \
            register long __arg3 asm ("r10") = (long)(arg3);        \
            register long __arg4 asm ("r8")  = (long)(arg4);        \
            register long __arg5 asm ("r9")  = (long)(arg5);        \
                                                                        \
            long __ret;                                                 \
            asm ("syscall"                             \
                                  : "=a" (__ret)                        \
                                  : "0" (__sysnum),                     \
                                    "r" (__arg0), "r" (__arg1),         \
                                    "r" (__arg2), "r" (__arg3),         \
                                    "r" (__arg4), "r" (__arg5)          \
                                  : "memory", "cc", "r11", "cx");       \
            __ret;                                                      \
        })

#define syscall6(sysnum, arg0, arg1, arg2, arg3, arg4, arg5)            \
    __syscall_XX(sysnum, arg0, arg1, arg2, arg3, arg4, arg5)
#define syscall5(sysnum, arg0, arg1, arg2, arg3, arg4)                  \
    __syscall_XX(sysnum, arg0, arg1, arg2, arg3, arg4, 0)
#define syscall4(sysnum, arg0, arg1, arg2, arg3)                        \
    __syscall_XX(sysnum, arg0, arg1, arg2, arg3, 0, 0)
#define syscall3(sysnum, arg0, arg1, arg2)                              \
    __syscall_XX(sysnum, arg0, arg1, arg2, 0, 0, 0)
#define syscall2(sysnum, arg0, arg1)                                    \
    __syscall_XX(sysnum, arg0, arg1, 0, 0, 0, 0)
#define syscall1(sysnum, arg0)                                          \
    __syscall_XX(sysnum, arg0, 0, 0, 0, 0, 0)
#define syscall0(sysnum)                                                \
    __syscall_XX(sysnum, 0, 0, 0, 0, 0, 0)


int flexSC_register(void);
int init_syscall_page(void);
int test_write(void);
long write_syscall(long args[], unsigned int syscall_num);
long wait_syscall(volatile int index);
long syscall_noflexsc(long *args, unsigned int sysnum);


#define MAX_ENTRY (128)

#define SYSCALL_ENTRY_FREE (0)
#define SYSCALL_ENTRY_SUBMITED (1)
#define SYSCALL_ENTRY_DONE (2)
#define SYSCALL_ENTRY_BLOCKED (3)

#endif
