#ifndef NOSTR_LINUX_X86_64_FSYNC_H_
#define NOSTR_LINUX_X86_64_FSYNC_H_

#include "../../../util/types.h"
#include "../errno.h"
#include "./asm.h"

static inline int32_t linux_x8664_fsync(int32_t fd)
{
  int32_t ret = linux_x8664_asm_syscall1(
    __NR_fsync,
    fd);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

static inline int32_t linux_x8664_fdatasync(int32_t fd)
{
  int32_t ret = linux_x8664_asm_syscall1(
    __NR_fdatasync,
    fd);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

#endif
