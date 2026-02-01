#ifndef NOSTR_LINUX_X86_64_OPEN_H_
#define NOSTR_LINUX_X86_64_OPEN_H_

#include "../../../util/types.h"
#include "../errno.h"
#include "../fcntl_def.h"
#include "./asm.h"

static inline int32_t linux_x8664_open(const char* pathname, int32_t flags, uint32_t mode)
{
  int32_t ret = linux_x8664_asm_syscall3(
    __NR_open,
    pathname,
    flags,
    mode);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

static inline int32_t linux_x8664_openat(int32_t dirfd, const char* pathname, int32_t flags, uint32_t mode)
{
  int32_t ret = linux_x8664_asm_syscall4(
    __NR_openat,
    dirfd,
    pathname,
    flags,
    mode);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

#endif
