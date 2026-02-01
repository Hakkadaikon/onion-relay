#ifndef NOSTR_LINUX_STAT_DEF_H_
#define NOSTR_LINUX_STAT_DEF_H_

#include "../../util/types.h"

// stat structure for x86_64 Linux
typedef struct {
  uint64_t st_dev;
  uint64_t st_ino;
  uint64_t st_nlink;
  uint32_t st_mode;
  uint32_t st_uid;
  uint32_t st_gid;
  uint32_t __pad0;
  uint64_t st_rdev;
  int64_t  st_size;
  int64_t  st_blksize;
  int64_t  st_blocks;
  int64_t  st_atime;
  int64_t  st_atime_nsec;
  int64_t  st_mtime;
  int64_t  st_mtime_nsec;
  int64_t  st_ctime;
  int64_t  st_ctime_nsec;
  int64_t  __unused[3];
} LinuxStat;

#endif
