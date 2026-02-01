#ifndef NOSTR_INTERNAL_OPEN_H_
#define NOSTR_INTERNAL_OPEN_H_

#include "linux/x86_64/open.h"

static inline int32_t internal_open(const char* pathname, int32_t flags, uint32_t mode)
{
  return linux_x8664_open(pathname, flags, mode);
}

static inline int32_t internal_openat(int32_t dirfd, const char* pathname, int32_t flags, uint32_t mode)
{
  return linux_x8664_openat(dirfd, pathname, flags, mode);
}

#endif
