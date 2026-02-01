#ifndef NOSTR_INTERNAL_FSTAT_H_
#define NOSTR_INTERNAL_FSTAT_H_

#include "linux/x86_64/fstat.h"

static inline int32_t internal_fstat(int32_t fd, LinuxStat* statbuf)
{
  return linux_x8664_fstat(fd, statbuf);
}

static inline int32_t internal_ftruncate(int32_t fd, int64_t length)
{
  return linux_x8664_ftruncate(fd, length);
}

#endif
