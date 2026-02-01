#ifndef NOSTR_LINUX_MMAP_DEF_H_
#define NOSTR_LINUX_MMAP_DEF_H_

// mmap protection flags
#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

// mmap flags
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_FILE 0x00

// mremap flags
#define MREMAP_MAYMOVE 0x1
#define MREMAP_FIXED 0x2

// msync flags
#define MS_ASYNC 0x1
#define MS_INVALIDATE 0x2
#define MS_SYNC 0x4

// madvise flags
#define MADV_NORMAL 0
#define MADV_RANDOM 1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED 3
#define MADV_DONTNEED 4

// Error value
#define MAP_FAILED ((void*)-1)

#endif
