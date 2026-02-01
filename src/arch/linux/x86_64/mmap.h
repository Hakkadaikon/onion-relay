#ifndef NOSTR_LINUX_X86_64_MMAP_H_
#define NOSTR_LINUX_X86_64_MMAP_H_

#include "../../../util/types.h"
#include "../errno.h"
#include "../mmap_def.h"
#include "./asm.h"

static inline void* linux_x8664_mmap(void* addr, size_t length, int32_t prot, int32_t flags, int32_t fd, int64_t offset)
{
  int64_t ret = linux_x8664_asm_syscall6(
    __NR_mmap,
    addr,
    length,
    prot,
    flags,
    fd,
    offset);

  if (ret < 0 && ret > -4096) {
    errno = (int32_t)(-ret);
    return MAP_FAILED;
  }
  return (void*)ret;
}

static inline int32_t linux_x8664_munmap(void* addr, size_t length)
{
  int32_t ret = linux_x8664_asm_syscall2(
    __NR_munmap,
    addr,
    length);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

static inline int32_t linux_x8664_msync(void* addr, size_t length, int32_t flags)
{
  int32_t ret = linux_x8664_asm_syscall3(
    __NR_msync,
    addr,
    length,
    flags);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

static inline int32_t linux_x8664_madvise(void* addr, size_t length, int32_t advice)
{
  int32_t ret = linux_x8664_asm_syscall3(
    __NR_madvise,
    addr,
    length,
    advice);

  SYSCALL_EARLY_RETURN(ret);
  return ret;
}

static inline void* linux_x8664_mremap(void* old_addr, size_t old_size, size_t new_size, int32_t flags)
{
  int64_t ret = linux_x8664_asm_syscall4(
    __NR_mremap,
    old_addr,
    old_size,
    new_size,
    flags);

  if (ret < 0 && ret > -4096) {
    errno = (int32_t)(-ret);
    return MAP_FAILED;
  }
  return (void*)ret;
}

#endif
