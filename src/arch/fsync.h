#ifndef NOSTR_INTERNAL_FSYNC_H_
#define NOSTR_INTERNAL_FSYNC_H_

#include "linux/x86_64/fsync.h"

static inline int32_t internal_fsync(int32_t fd)
{
  return linux_x8664_fsync(fd);
}

static inline int32_t internal_fdatasync(int32_t fd)
{
  return linux_x8664_fdatasync(fd);
}

#endif
