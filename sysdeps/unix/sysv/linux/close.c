/* Linux close syscall implementation.
   Copyright (C) 2017-2018 Free Software Foundation, Inc.
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

#include <unistd.h>
#include <sysdep-cancel.h>
#include <not-cancel.h>

/* Close the file descriptor FD.  */
int
__close (int fd)
{
  return SYSCALL_CANCEL (close, fd);
}
libc_hidden_def (__close)
strong_alias (__close, __libc_close)
weak_alias (__close, close)

# if !IS_IN (rtld)
int
__close_nocancel (int fd)
{
  return INLINE_SYSCALL_CALL (close, fd);
}
#else
strong_alias (__libc_close, __close_nocancel)
#endif
libc_hidden_def (__close_nocancel)
