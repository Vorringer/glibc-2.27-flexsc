/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <sys/syscall.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sysdep.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>


#undef	atoi
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
/* Convert a string to an int.  */
const char *syscall_file = "/dev/shmem_dev";
int fd = -1;
#define MAX_ENTRY (64)
#define SYSCALL_ENTRY_FREE (0)
#define SYSCALL_ENTRY_SUBMITED (1)
#define SYSCALL_ENTRY_DONE (2)
#define SYSCALL_ENTRY_BLOCKED (3)

#define __syscalll_XX(sysnum, arg0, arg1, arg2, arg3, arg4, arg5) ({     \
            int __sysnum = (int)(sysnum);                               \
            register long __arg0 asm ("rdi") = (long)(arg0);        \
            register long __arg1 asm ("rsi") = (long)(arg1);        \
            register long __arg2 asm ("rdx") = (long)(arg2);        \
            register long __arg3 asm ("r10") = (long)(arg3);        \
            register long __arg4 asm ("r8")  = (long)(arg4);        \
            register long __arg5 asm ("r9")  = (long)(arg5);        \
                                                                        \
            long __ret;                                                 \
            asm volatile("syscall"                             \
                                  : "=a" (__ret)                        \
                                  : "0" (__sysnum),                     \
                                    "r" (__arg0), "r" (__arg1),         \
                                    "r" (__arg2), "r" (__arg3),         \
                                    "r" (__arg4), "r" (__arg5)          \
                                  : "memory", "cc", "r11", "cx");       \
            __ret;                                                      \
        })

#define syscalll6(sysnum, arg0, arg1, arg2, arg3, arg4, arg5)            \
    __syscalll_XX(sysnum, arg0, arg1, arg2, arg3, arg4, arg5)
#define syscalll5(sysnum, arg0, arg1, arg2, arg3, arg4)                  \
    __syscalll_XX(sysnum, arg0, arg1, arg2, arg3, arg4, 0)
#define syscalll4(sysnum, arg0, arg1, arg2, arg3)                        \
    __syscalll_XX(sysnum, arg0, arg1, arg2, arg3, 0, 0)
#define syscalll3(sysnum, arg0, arg1, arg2)                              \
    __syscalll_XX(sysnum, arg0, arg1, arg2, 0, 0, 0)
#define syscalll2(sysnum, arg0, arg1)                                    \
    __syscalll_XX(sysnum, arg0, arg1, 0, 0, 0, 0)
#define syscalll1(sysnum, arg0)                                          \
    __syscalll_XX(sysnum, arg0, 0, 0, 0, 0, 0)
#define syscalll0(sysnum)                                                \
    __syscalll_XX(sysnum, 0, 0, 0, 0, 0, 0)

typedef long (*flexSC_syscall_t)(long *sysargs, unsigned int sysnum);

long
syscall_noflexsc(long *args, unsigned int sysnum) {
    return syscalll6(sysnum,
                    args[0], args[1], args[2],
                    args[3], args[4], args[5]);
}


volatile flexSC_syscall_t __flexsc_syscall_handle = syscall_noflexsc;

//#pragma GCC push_options
//#pragma GCC optimize ("O0")
long check_and_fill(int i, long args[], Syscall_entry *syscall_page, unsigned int syscall_num) {
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
      return -1;
}
//#pragma GCC pop_options

long write_syscall(long args[], unsigned int syscall_num) {
   int index = -1;
   Syscall_entry *syscall_page = (Syscall_entry *)_syscall_page;
   while (1){
      for (int i = 0; i < MAX_ENTRY; ++i) {
         index = check_and_fill(i, args, syscall_page, syscall_num);
         if (index >= 0)
            goto out;
      }
   }
out:
   if (index == -1) return -1;
   while (syscall_page[index].status != SYSCALL_ENTRY_DONE) __sync_synchronize();
   long ret = (unsigned long)syscall_page[index].ret_value;

   syscall_page[index].status = SYSCALL_ENTRY_FREE;
   return ret;
}

long flexSC_register(void) {
   if (FLEXSC_REGISTERED) return 0;
   int page_size;
   long ret;
   page_size = 1 << 12;
   fd = open(syscall_file, O_RDWR | O_SYNC);
    if (fd < 0) {
        return -1;
    }
    _syscall_page = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!_syscall_page) {
      return -1;
    }
    ret = 1;
    __flexsc_syscall_handle = write_syscall;
    register long arg1 asm ("rdi") = syscalll0(39);
    asm volatile (
    "syscall\n\t"
    : "=a" (ret)
    : "0" (332), "r" (arg1)
    : "memory", "cc", "r11", "cx");

    FLEXSC_REGISTERED = 1;


   return 0;
}

long flexSC_mtest(long len) {
   INTERNAL_SYSCALL_DECL (err);
   return INTERNAL_SYSCALL_NCS(334, err, 1, len);
}

long flexSC_cancel(int mode) {
   long ret = 0;
   if (mode == 0) {
      munmap((void *)_syscall_page, 1 << 12);
      _syscall_page = NULL;
      close(fd);
      asm volatile (
       "syscall\n\t"
       : "=a" (ret)
       : "0" (333)
       : "memory", "cc", "r11", "cx");
   }
   __flexsc_syscall_handle = syscall_noflexsc;
   FLEXSC_REGISTERED = 0;
   return 0;
}

int
atoi (const char *nptr)
{
  return (int) strtol (nptr, (char **) NULL, 10);
}
