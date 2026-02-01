#ifndef NOSTR_LINUX_X86_64_FSTAT_H_
#define NOSTR_LINUX_X86_64_FSTAT_H_

#include "../../../util/types.h"
#include "../errno.h"
#include "../stat_def.h"
#include "./asm.h"

static inline int32_t linux_x8664_fstat(int32_t fd, LinuxStat* statbuf)
{
  int32_t ret = linux_x8664_asm_syscall2(
    __NR_fstat,
    fd,
    statbuf);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

static inline int32_t linux_x8664_ftruncate(int32_t fd, int64_t length)
{
  int32_t ret = linux_x8664_asm_syscall2(
    __NR_ftruncate,
    fd,
    length);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

#endif
