#ifndef NOSTR_INTERNAL_FILE_READ_H_
#define NOSTR_INTERNAL_FILE_READ_H_

#include "linux/x86_64/file_read.h"

static inline ssize_t internal_file_read(int32_t fd, void* buf, size_t count)
{
  return linux_x8664_read(fd, buf, count);
}

static inline ssize_t internal_pread(int32_t fd, void* buf, size_t count, int64_t offset)
{
  return linux_x8664_pread(fd, buf, count, offset);
}

#endif
