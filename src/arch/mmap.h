#ifndef NOSTR_INTERNAL_MMAP_H_
#define NOSTR_INTERNAL_MMAP_H_

#include "linux/x86_64/mmap.h"

static inline void* internal_mmap(void* addr, size_t length, int32_t prot, int32_t flags, int32_t fd, int64_t offset)
{
  return linux_x8664_mmap(addr, length, prot, flags, fd, offset);
}

static inline int32_t internal_munmap(void* addr, size_t length)
{
  return linux_x8664_munmap(addr, length);
}

static inline int32_t internal_msync(void* addr, size_t length, int32_t flags)
{
  return linux_x8664_msync(addr, length, flags);
}

static inline int32_t internal_madvise(void* addr, size_t length, int32_t advice)
{
  return linux_x8664_madvise(addr, length, advice);
}

static inline void* internal_mremap(void* old_addr, size_t old_size, size_t new_size, int32_t flags)
{
  return linux_x8664_mremap(old_addr, old_size, new_size, flags);
}

#endif
