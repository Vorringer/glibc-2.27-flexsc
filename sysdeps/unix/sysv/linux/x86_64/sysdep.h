/* Copyright (C) 2001-2018 Free Software Foundation, Inc.
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

#ifndef _LINUX_X86_64_SYSDEP_H
#define _LINUX_X86_64_SYSDEP_H 1

/* There is some commonality.  */
#include <sysdeps/unix/sysv/linux/sysdep.h>
#include <sysdeps/unix/x86_64/sysdep.h>
#include <tls.h>
#if IS_IN (rtld)
# include <dl-sysdep.h>		/* Defines RTLD_PRIVATE_ERRNO.  */
#endif


/* For Linux we can use the system call table in the header file
	/usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#define SYS_ify(syscall_name)	__NR_##syscall_name

/* This is a kludge to make syscalls.list find these under the names
   pread and pwrite, since some kernel headers define those names
   and some define the *64 names for the same system calls.  */
#if !defined __NR_pread && defined __NR_pread64
# define __NR_pread __NR_pread64
#endif
#if !defined __NR_pwrite && defined __NR_pwrite64
# define __NR_pwrite __NR_pwrite64
#endif

/* This is to help the old kernel headers where __NR_semtimedop is not
   available.  */
#ifndef __NR_semtimedop
# define __NR_semtimedop 220
#endif


#ifdef __ASSEMBLER__
/* Linux uses a negative return value to indicate syscall errors,
   unlike most Unices, which use the condition codes' carry flag.

   Since version 2.1 the return value of a system call might be
   negative even if the call succeeded.	 E.g., the `lseek' system call
   might return a large offset.	 Therefore we must not anymore test
   for < 0, but test for a real error by making sure the value in %eax
   is a real error number.  Linus said he will make sure the no syscall
   returns a value in -1 .. -4095 as a valid result so we can savely
   test with -4095.  */

/* We don't want the label for the error handle to be global when we define
   it here.  */
# ifdef PIC
#  define SYSCALL_ERROR_LABEL 0f
# else
#  define SYSCALL_ERROR_LABEL syscall_error
# endif

# undef	PSEUDO
# define PSEUDO(name, syscall_name, args)				      \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    cmpq $-4095, %rax;							      \
    jae SYSCALL_ERROR_LABEL

# undef	PSEUDO_END
# define PSEUDO_END(name)						      \
  SYSCALL_ERROR_HANDLER							      \
  END (name)

# undef	PSEUDO_NOERRNO
# define PSEUDO_NOERRNO(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args)

# undef	PSEUDO_END_NOERRNO
# define PSEUDO_END_NOERRNO(name) \
  END (name)

# define ret_NOERRNO ret

# undef	PSEUDO_ERRVAL
# define PSEUDO_ERRVAL(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    negq %rax

# undef	PSEUDO_END_ERRVAL
# define PSEUDO_END_ERRVAL(name) \
  END (name)

# define ret_ERRVAL ret

# if defined PIC && defined RTLD_PRIVATE_ERRNO
#  define SYSCALL_SET_ERRNO			\
  lea rtld_errno(%rip), %RCX_LP;		\
  neg %eax;					\
  movl %eax, (%rcx)
# else
#  if IS_IN (libc)
#   define SYSCALL_ERROR_ERRNO __libc_errno
#  else
#   define SYSCALL_ERROR_ERRNO errno
#  endif
#  define SYSCALL_SET_ERRNO			\
  movq SYSCALL_ERROR_ERRNO@GOTTPOFF(%rip), %rcx;\
  neg %eax;					\
  movl %eax, %fs:(%rcx);
# endif

# ifndef PIC
#  define SYSCALL_ERROR_HANDLER	/* Nothing here; code in sysdep.S is used.  */
# else
#  define SYSCALL_ERROR_HANDLER			\
0:						\
  SYSCALL_SET_ERRNO;				\
  or $-1, %RAX_LP;				\
  ret;
# endif	/* PIC */

/* The Linux/x86-64 kernel expects the system call parameters in
   registers according to the following table:

    syscall number	rax
    arg 1		rdi
    arg 2		rsi
    arg 3		rdx
    arg 4		r10
    arg 5		r8
    arg 6		r9

    The Linux kernel uses and destroys internally these registers:
    return address from
    syscall		rcx
    eflags from syscall	r11

    Normal function call, including calls to the system call stub
    functions in the libc, get the first six parameters passed in
    registers and the seventh parameter and later on the stack.  The
    register use is as follows:

     system call number	in the DO_CALL macro
     arg 1		rdi
     arg 2		rsi
     arg 3		rdx
     arg 4		rcx
     arg 5		r8
     arg 6		r9

    We have to take care that the stack is aligned to 16 bytes.  When
    called the stack is not aligned since the return address has just
    been pushed.


    Syscalls of more than 6 arguments are not supported.  */

.extern long hehehe;

# undef	DO_CALL
# define DO_CALL(syscall_name, args)		\
    DOARGS_##args				                  \
    movl hehehe@GOTPCREL(%rip), %eax;   \
    cmpl $2, %eax;                      \
    je L22;                             \
    movl $SYS_ify (syscall_name), %eax;		\
    syscall;                             \
    L22:

# define DOARGS_0 /* nothing */
# define DOARGS_1 /* nothing */
# define DOARGS_2 /* nothing */
# define DOARGS_3 /* nothing */
# define DOARGS_4 movq %rcx, %r10;
# define DOARGS_5 DOARGS_4
# define DOARGS_6 DOARGS_5

#else	/* !__ASSEMBLER__ */
/* Define a macro which expands inline into the wrapper code for a system
   call.  */
# undef INLINE_SYSCALL
# define INLINE_SYSCALL(name, nr, args...) \
  ({									      \
    unsigned long int resultvar = INTERNAL_SYSCALL (name, , nr, args);	      \
    if (__glibc_unlikely (INTERNAL_SYSCALL_ERROR_P (resultvar, )))	      \
      {									      \
	__set_errno (INTERNAL_SYSCALL_ERRNO (resultvar, ));		      \
	resultvar = (unsigned long int) -1;				      \
      }									      \
    (long int) resultvar; })

/* Define a macro with explicit types for arguments, which expands inline
   into the wrapper code for a system call.  It should be used when size
   of any argument > size of long int.  */
# undef INLINE_SYSCALL_TYPES
# define INLINE_SYSCALL_TYPES(name, nr, args...) \
  ({									      \
    unsigned long int resultvar = INTERNAL_SYSCALL_TYPES (name, , nr, args);  \
    if (__glibc_unlikely (INTERNAL_SYSCALL_ERROR_P (resultvar, )))	      \
      {									      \
	__set_errno (INTERNAL_SYSCALL_ERRNO (resultvar, ));		      \
	resultvar = (unsigned long int) -1;				      \
      }									      \
    (long int) resultvar; })

# undef INTERNAL_SYSCALL_DECL
# define INTERNAL_SYSCALL_DECL(err) do { } while (0)

/* Registers clobbered by syscall.  */
# define REGISTERS_CLOBBERED_BY_SYSCALL "cc", "r11", "cx"

/* Create a variable 'name' based on type 'X' to avoid explicit types.
   This is mainly used set use 64-bits arguments in x32.   */
#define TYPEFY(X, name) __typeof__ ((X) - (X)) name
/* Explicit cast the argument to avoid integer from pointer warning on
   x32.  */
#define ARGIFY(X) ((__typeof__ ((X) - (X))) (X))

#undef INTERNAL_SYSCALL
#define INTERNAL_SYSCALL(name, err, nr, args...)			\
	internal_syscall##nr (SYS_ify (name), err, args)

#undef INTERNAL_SYSCALL_NCS
#define INTERNAL_SYSCALL_NCS(number, err, nr, args...)			\
	internal_syscall##nr (number, err, args)

#undef internal_syscall0
#define internal_syscall0(number, err, dummy...)      \
({                  \
  unsigned long int resultvar = 0;        \
  extern void *_syscall_page;       \
  extern int FLEXSC_REGISTERED;     \
  if (FLEXSC_REGISTERED) {          \
    int i, index;   \
    i = 0;    \
    index = -1;   \
    for (i = 0; i < 64; ++i) {   \
      if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
        *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
        *(long *)(_syscall_page + (i << 6)) = number;   \
        *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
        index = i;    \
        break;    \
      }     \
    }   \
    if (index != -1) {      \
      while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
      resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
      *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
    }   \
  }           \
  else {        \
    asm volatile (              \
    "syscall\n\t"             \
    : "=a" (resultvar)              \
    : "0" (number)              \
    : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
  }         \
  (long int) resultvar;       \
})

#undef internal_syscall1
#define internal_syscall1(number, err, arg1)        \
({                  \
    unsigned long int resultvar = 0;          \
    extern void *_syscall_page;       \
    extern int FLEXSC_REGISTERED;     \
    if (FLEXSC_REGISTERED) {          \
      int i;    \
      volatile int index;   \
      i = 0;    \
      index = -1;   \
      for (i = 0; i < 64; ++i) {   \
        if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
          *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
          *(long *)(_syscall_page + (i << 6)) = number;   \
          *(long *)(_syscall_page + (i << 6) + 16) = (long)(arg1);    \
          *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
          index = i;    \
          break;    \
        }     \
      }   \
      if (index != -1) {      \
        while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
        resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
        *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
      }   \
    }           \
    else {    \
      TYPEFY (arg1, __arg1) = ARGIFY (arg1);        \
      register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;     \
      asm volatile (              \
      "syscall\n\t"             \
      : "=a" (resultvar)              \
      : "0" (number), "r" (_a1)           \
      : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
    }     \
    (long int) resultvar;           \
})

#undef internal_syscall2
#define internal_syscall2(number, err, arg1, arg2)      \
({                  \
    unsigned long int resultvar = 0;          \
    extern void *_syscall_page;       \
    extern int FLEXSC_REGISTERED;     \
    if (FLEXSC_REGISTERED) {          \
      int i;    \
      volatile int index;   \
      i = 0;    \
      index = -1;   \
      for (i = 0; i < 64; ++i) {   \
        if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
          *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
          *(long *)(_syscall_page + (i << 6)) = number;   \
          *(long *)(_syscall_page + (i << 6) + 16) = (long)(arg1);    \
          *(long *)(_syscall_page + (i << 6) + 24) = (long)(arg2);    \
          *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
          index = i;    \
          break;    \
        }     \
      }   \
      if (index != -1) {      \
        while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
        resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
        *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
      }   \
    }           \
    else {    \
      TYPEFY (arg2, __arg2) = ARGIFY (arg2);        \
      TYPEFY (arg1, __arg1) = ARGIFY (arg1);        \
      register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;     \
      register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;     \
      asm volatile (              \
      "syscall\n\t"             \
      : "=a" (resultvar)              \
      : "0" (number), "r" (_a1), "r" (_a2)        \
      : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
    }\
    (long int) resultvar;           \
})

#undef internal_syscall3
#define internal_syscall3(number, err, arg1, arg2, arg3)    \
({                  \
    unsigned long int resultvar = 0;          \
    extern void *_syscall_page;       \
    extern int FLEXSC_REGISTERED;     \
    if (FLEXSC_REGISTERED) {          \
      int i;    \
      volatile int index;   \
      i = 0;    \
      index = -1;   \
      for (i = 0; i < 64; ++i) {   \
        if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
          *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
          *(long *)(_syscall_page + (i << 6)) = number;   \
          *(long *)(_syscall_page + (i << 6) + 16) = (long)(arg1);    \
          *(long *)(_syscall_page + (i << 6) + 24) = (long)(arg2);    \
          *(long *)(_syscall_page + (i << 6) + 32) = (long)(arg3);    \
          *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
          index = i;    \
          break;    \
        }     \
      }   \
      if (index != -1) {      \
        while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
        resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
        *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
      }   \
    }           \
    else {    \
      TYPEFY (arg3, __arg3) = ARGIFY (arg3);        \
      TYPEFY (arg2, __arg2) = ARGIFY (arg2);        \
      TYPEFY (arg1, __arg1) = ARGIFY (arg1);        \
      register TYPEFY (arg3, _a3) asm ("rdx") = __arg3;     \
      register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;     \
      register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;     \
      asm volatile (              \
      "syscall\n\t"             \
      : "=a" (resultvar)              \
      : "0" (number), "r" (_a1), "r" (_a2), "r" (_a3)     \
      : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
    }   \
    (long int) resultvar;           \
})

#undef internal_syscall4
#define internal_syscall4(number, err, arg1, arg2, arg3, arg4)    \
({                  \
    unsigned long int resultvar = 0;          \
    extern void *_syscall_page;       \
    extern int FLEXSC_REGISTERED;     \
    if (FLEXSC_REGISTERED) {          \
      int i;    \
      volatile int index;   \
      i = 0;    \
      index = -1;   \
      for (i = 0; i < 64; ++i) {   \
        if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
          *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
          *(long *)(_syscall_page + (i << 6)) = number;   \
          *(long *)(_syscall_page + (i << 6) + 16) = (long)(arg1);    \
          *(long *)(_syscall_page + (i << 6) + 24) = (long)(arg2);    \
          *(long *)(_syscall_page + (i << 6) + 32) = (long)(arg3);    \
          *(long *)(_syscall_page + (i << 6) + 40) = (long)(arg4);    \
          *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
          index = i;    \
          break;    \
        }     \
      }   \
      if (index != -1) {      \
        while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
        resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
        *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
      }   \
    }           \
    else {    \
      TYPEFY (arg4, __arg4) = ARGIFY (arg4);        \
      TYPEFY (arg3, __arg3) = ARGIFY (arg3);        \
      TYPEFY (arg2, __arg2) = ARGIFY (arg2);        \
      TYPEFY (arg1, __arg1) = ARGIFY (arg1);        \
      register TYPEFY (arg4, _a4) asm ("r10") = __arg4;     \
      register TYPEFY (arg3, _a3) asm ("rdx") = __arg3;     \
      register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;     \
      register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;     \
      asm volatile (              \
      "syscall\n\t"             \
      : "=a" (resultvar)              \
      : "0" (number), "r" (_a1), "r" (_a2), "r" (_a3), "r" (_a4)    \
      : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
    }   \
    (long int) resultvar;           \
})

#undef internal_syscall5
#define internal_syscall5(number, err, arg1, arg2, arg3, arg4, arg5)  \
({                  \
    unsigned long int resultvar = 0;          \
    extern void *_syscall_page;       \
    extern int FLEXSC_REGISTERED;     \
    if (FLEXSC_REGISTERED) {          \
      int i;    \
      volatile int index;   \
      i = 0;    \
      index = -1;   \
      for (i = 0; i < 64; ++i) {   \
        if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
          *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
          *(long *)(_syscall_page + (i << 6)) = number;   \
          *(long *)(_syscall_page + (i << 6) + 16) = (long)(arg1);    \
          *(long *)(_syscall_page + (i << 6) + 24) = (long)(arg2);    \
          *(long *)(_syscall_page + (i << 6) + 32) = (long)(arg3);    \
          *(long *)(_syscall_page + (i << 6) + 40) = (long)(arg4);    \
          *(long *)(_syscall_page + (i << 6) + 48) = (long)(arg5);    \
          *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
          index = i;    \
          break;    \
        }     \
      }   \
      if (index != -1) {      \
        while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
        resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
        *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
      }   \
    }           \
    else {    \
      TYPEFY (arg5, __arg5) = ARGIFY (arg5);        \
      TYPEFY (arg4, __arg4) = ARGIFY (arg4);        \
      TYPEFY (arg3, __arg3) = ARGIFY (arg3);        \
      TYPEFY (arg2, __arg2) = ARGIFY (arg2);        \
      TYPEFY (arg1, __arg1) = ARGIFY (arg1);        \
      register TYPEFY (arg5, _a5) asm ("r8") = __arg5;      \
      register TYPEFY (arg4, _a4) asm ("r10") = __arg4;     \
      register TYPEFY (arg3, _a3) asm ("rdx") = __arg3;     \
      register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;     \
      register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;     \
      asm volatile (              \
      "syscall\n\t"             \
      : "=a" (resultvar)              \
      : "0" (number), "r" (_a1), "r" (_a2), "r" (_a3), "r" (_a4),   \
        "r" (_a5)               \
      : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
    }     \
    (long int) resultvar;           \
})

#undef internal_syscall6
#define internal_syscall6(number, err, arg1, arg2, arg3, arg4, arg5, arg6) \
({                  \
    unsigned long int resultvar = 0;          \
    extern void *_syscall_page;       \
    extern int FLEXSC_REGISTERED;     \
    if (FLEXSC_REGISTERED) {          \
      int i;    \
      volatile int index;   \
      i = 0;    \
      index = -1;   \
      for (i = 0; i < 64; ++i) {   \
        if (*(short *)(_syscall_page + (i << 6) + 6) == 0) {    \
          *(short *)(_syscall_page + (i << 6) + 6) = 3;   \
          *(long *)(_syscall_page + (i << 6)) = number;   \
          *(long *)(_syscall_page + (i << 6) + 16) = (long)(arg1);    \
          *(long *)(_syscall_page + (i << 6) + 24) = (long)(arg2);    \
          *(long *)(_syscall_page + (i << 6) + 32) = (long)(arg3);    \
          *(long *)(_syscall_page + (i << 6) + 40) = (long)(arg4);    \
          *(long *)(_syscall_page + (i << 6) + 48) = (long)(arg5);    \
          *(long *)(_syscall_page + (i << 6) + 56) = (long)(arg6);    \
          *(short *)(_syscall_page + (i << 6) + 6) = 1;   \
          index = i;    \
          break;    \
        }     \
      }   \
      if (index != -1) {      \
        while (*(short *)(_syscall_page + (index << 6) + 6) != 2) asm("nop");    \
        resultvar = *(long *)(_syscall_page + (index << 6) + 8);    \
        *(short *)(_syscall_page + (index << 6) + 6) = 0;   \
      }   \
    }           \
    else {    \
      TYPEFY (arg6, __arg6) = ARGIFY (arg6);        \
      TYPEFY (arg5, __arg5) = ARGIFY (arg5);        \
      TYPEFY (arg4, __arg4) = ARGIFY (arg4);        \
      TYPEFY (arg3, __arg3) = ARGIFY (arg3);        \
      TYPEFY (arg2, __arg2) = ARGIFY (arg2);        \
      TYPEFY (arg1, __arg1) = ARGIFY (arg1);        \
      register TYPEFY (arg6, _a6) asm ("r9") = __arg6;      \
      register TYPEFY (arg5, _a5) asm ("r8") = __arg5;      \
      register TYPEFY (arg4, _a4) asm ("r10") = __arg4;     \
      register TYPEFY (arg3, _a3) asm ("rdx") = __arg3;     \
      register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;     \
      register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;     \
      asm volatile (              \
      "syscall\n\t"             \
      : "=a" (resultvar)              \
      : "0" (number), "r" (_a1), "r" (_a2), "r" (_a3), "r" (_a4),   \
        "r" (_a5), "r" (_a6)            \
      : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);      \
    }   \
    (long int) resultvar;           \
})

# undef INTERNAL_SYSCALL_ERROR_P
# define INTERNAL_SYSCALL_ERROR_P(val, err) \
  ((unsigned long int) (long int) (val) >= -4095L)

# undef INTERNAL_SYSCALL_ERRNO
# define INTERNAL_SYSCALL_ERRNO(val, err)	(-(val))

/* List of system calls which are supported as vsyscalls.  */
# define HAVE_CLOCK_GETTIME_VSYSCALL    1
# define HAVE_GETTIMEOFDAY_VSYSCALL     1
# define HAVE_GETCPU_VSYSCALL		1

# define SINGLE_THREAD_BY_GLOBAL		1

#endif	/* __ASSEMBLER__ */


/* Pointer mangling support.  */
#if IS_IN (rtld)
/* We cannot use the thread descriptor because in ld.so we use setjmp
   earlier than the descriptor is initialized.  */
# ifdef __ASSEMBLER__
#  define PTR_MANGLE(reg)	xor __pointer_chk_guard_local(%rip), reg;    \
				rol $2*LP_SIZE+1, reg
#  define PTR_DEMANGLE(reg)	ror $2*LP_SIZE+1, reg;			     \
				xor __pointer_chk_guard_local(%rip), reg
# else
#  define PTR_MANGLE(reg)	asm ("xor __pointer_chk_guard_local(%%rip), %0\n" \
				     "rol $2*" LP_SIZE "+1, %0"			  \
				     : "=r" (reg) : "0" (reg))
#  define PTR_DEMANGLE(reg)	asm ("ror $2*" LP_SIZE "+1, %0\n"		  \
				     "xor __pointer_chk_guard_local(%%rip), %0"   \
				     : "=r" (reg) : "0" (reg))
# endif
#else
# ifdef __ASSEMBLER__
#  define PTR_MANGLE(reg)	xor %fs:POINTER_GUARD, reg;		      \
				rol $2*LP_SIZE+1, reg
#  define PTR_DEMANGLE(reg)	ror $2*LP_SIZE+1, reg;			      \
				xor %fs:POINTER_GUARD, reg
# else
#  define PTR_MANGLE(var)	asm ("xor %%fs:%c2, %0\n"		      \
				     "rol $2*" LP_SIZE "+1, %0"		      \
				     : "=r" (var)			      \
				     : "0" (var),			      \
				       "i" (offsetof (tcbhead_t,	      \
						      pointer_guard)))
#  define PTR_DEMANGLE(var)	asm ("ror $2*" LP_SIZE "+1, %0\n"	      \
				     "xor %%fs:%c2, %0"			      \
				     : "=r" (var)			      \
				     : "0" (var),			      \
				       "i" (offsetof (tcbhead_t,	      \
						      pointer_guard)))
# endif
#endif

/* How to pass the off{64}_t argument on p{readv,writev}{64}.  */
#undef LO_HI_LONG
#define LO_HI_LONG(val) (val), 0

#endif /* linux/x86_64/sysdep.h */
