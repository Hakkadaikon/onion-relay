#ifndef NOSTR_LINUX_X86_64_FILE_READ_H_
#define NOSTR_LINUX_X86_64_FILE_READ_H_

#include "../../../util/types.h"
#include "../errno.h"
#include "./asm.h"

static inline ssize_t linux_x8664_read(int32_t fd, void* buf, size_t count)
{
  int32_t ret = linux_x8664_asm_syscall3(
    __NR_read,
    fd,
    buf,
    count);

  SYSCALL_SIZE_EARLY_RETURN(ret);
  return (ssize_t)ret;
}

static inline ssize_t linux_x8664_pread(int32_t fd, void* buf, size_t count, int64_t offset)
{
  int32_t ret = linux_x8664_asm_syscall4(
    __NR_pread64,
    fd,
    buf,
    count,
    offset);

  SYSCALL_SIZE_EARLY_RETURN(ret);
  return (ssize_t)ret;
}

#endif
