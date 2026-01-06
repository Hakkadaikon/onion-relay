#ifndef NOSTR_LINUX_X86_64_MEMORY_H_
#define NOSTR_LINUX_X86_64_MEMORY_H_

#include "../../../util/string.h"
#include "../../../util/types.h"
#include "../errno.h"

static inline void* linux_x8664_memcpy(void* dest, const void* src, size_t size)
{
  require_not_null(dest, NULL);
  require_not_null(src, NULL);
  require_valid_length(size, NULL);

  unsigned char*       d = (unsigned char*)dest;
  const unsigned char* s = (const unsigned char*)src;
  size_t               n = size;

  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

static inline void* linux_x8664_memset(void* s, const int c, const size_t size)
{
  require_not_null(s, NULL);
  require_valid_length(size, NULL);

  unsigned char* p = (unsigned char*)s;
  size_t         n = size;
  while (n--) {
    *p++ = (unsigned char)c;
  }

  return s;
}

/*
 * memset_s: Secure memory set function (wrapper for memset)
 *
 * @s: Pointer to the memory area to be set
 * @smax: Size of the memory area in bytes
 * @c: Value to set (passed as int, but treated as an unsigned char)
 * @n: Number of bytes to set
 *
 * Note: A compiler barrier is inserted to ensure that the memset call is not optimized away.
 */
static inline int32_t linux_x8664_memset_s(void* s, const size_t smax, const int32_t c, const size_t n)
{
  require_not_null(s, EINVAL);
  require_valid_length(smax, EINVAL);
  require_valid_length(n, EINVAL);

  // If n is greater than smax, clear the entire buffer (if possible) and return an error
  if (n > smax) {
    linux_x8664_memset(s, c, smax);
    // Compiler barrier to prevent optimization removal
    __asm__ volatile("" ::
                       : "memory");
    return EINVAL;
  }

  // Set n bytes of memory to the value c
  linux_x8664_memset(s, c, n);
  // Compiler barrier to prevent optimization removal
  __asm__ volatile("" ::
                     : "memory");
  return 0;
}

#endif
