#ifndef NOSTR_LINUX_FCNTL_DEF_H_
#define NOSTR_LINUX_FCNTL_DEF_H_

// File access modes
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002

// File creation flags
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_NOCTTY    0x0100
#define O_TRUNC     0x0200

// File status flags
#define O_APPEND    0x0400
#define O_NONBLOCK  0x0800
#define O_DSYNC     0x1000
#define O_SYNC      0x101000
#define O_RSYNC     0x101000
#define O_DIRECTORY 0x10000
#define O_NOFOLLOW  0x20000
#define O_CLOEXEC   0x80000

// File mode bits
#define S_IRWXU     0700    // Owner: read, write, execute
#define S_IRUSR     0400    // Owner: read
#define S_IWUSR     0200    // Owner: write
#define S_IXUSR     0100    // Owner: execute

#define S_IRWXG     0070    // Group: read, write, execute
#define S_IRGRP     0040    // Group: read
#define S_IWGRP     0020    // Group: write
#define S_IXGRP     0010    // Group: execute

#define S_IRWXO     0007    // Others: read, write, execute
#define S_IROTH     0004    // Others: read
#define S_IWOTH     0002    // Others: write
#define S_IXOTH     0001    // Others: execute

#endif
