#ifndef NOSTR_STRING_H_
#define NOSTR_STRING_H_

#include "../util/types.h"

// For C++ compilation (tests): use standard library functions
#ifdef __cplusplus
#include <cstring>
// Use nostr_ prefix for custom functions that differ from standard
#define NOSTR_USE_CUSTOM_STRING_FUNCS 0
#else
// For C compilation (production): use custom implementations
#define NOSTR_USE_CUSTOM_STRING_FUNCS 1
#endif

#define value(ptr) (*ptr)
#define is_null(str) (str == NULL)
#define require_not_null(ptr, ret_val) \
  if (is_null(ptr)) {                  \
    return ret_val;                    \
  }
#define require_not_null_or_empty(ptr, ret_val) \
  if (is_null_or_empty(ptr)) {                  \
    return ret_val;                             \
  }
#define require_valid_length(len, ret_val) \
  if (len <= 0) {                          \
    return ret_val;                        \
  }
#define require(conditions, ret_val) \
  if (!(conditions)) {               \
    return ret_val;                  \
  }

static inline bool is_empty_value(const char c)
{
  return (c == '\0');
}

static inline bool is_empty(const char* str)
{
  return (is_empty_value(value(str)));
}

static inline bool is_null_or_empty(const char* str)
{
  return (is_null(str) || is_empty(str));
}

static inline bool is_line_break(const char c)
{
  return ((c == '\r') || (c == '\n'));
}

static inline bool is_space(const char c)
{
  return ((c == ' ') ||
          (c == '\t') ||
          (c == '\r') ||
          (c == '\n') ||
          (c == '\v') ||
          (c == '\f'));
}

static inline bool is_lower(const char c)
{
  return ((c >= 'a') && (c <= 'z'));
}

static inline bool is_upper(const char c)
{
  return ((c >= 'A') && (c <= 'Z'));
}

static inline bool is_digit(const char c)
{
  return ((c >= '0') && (c <= '9'));
}

static inline bool is_lower_hex(const char c)
{
  return is_digit(c) || ((c >= 'a') && (c <= 'f'));
}

static inline bool is_lower_hex_str(const char* c, const size_t len)
{
  require_not_null(c, false);
  require_valid_length(len, false);

  for (int i = 0; i < len; i++) {
    if (!is_lower_hex(c[i])) {
      return false;
    }
  }

  return true;
}

static inline bool is_utf8_space(const char* str)
{
  require_not_null(str, false);

  // UTF-8: 0xE3 0x80 0x80
  return (str[0] == '\xE3') &&
         (str[1] == '\x80') &&
         (str[2] == '\x80');
}

static inline bool chrcmp_sensitive(const char a, const char b)
{
  if (a != b) {
    return false;
  }

  return true;
}

static inline bool chrcmp(const char a, const char b)
{
  char lower_a = (a >= 'A' && a <= 'Z') ? (a + 'a' - 'A') : (a);
  char lower_b = (b >= 'A' && b <= 'Z') ? (b + 'a' - 'A') : (b);

  if (lower_a != lower_b) {
    return false;
  }

  return true;
}

// Custom strncmp: case-insensitive comparison, returns bool
// Different from standard strncmp which is case-sensitive and returns int
static inline bool nostr_strncmp(
  const char*  str1,
  const char*  str2,
  const size_t capacity)
{
  require_not_null(str1, false);
  require_not_null(str2, false);
  require_valid_length(capacity, false);

  for (size_t i = 0; i < capacity; i++) {
    if (!chrcmp(str1[i], str2[i])) {
      return false;
    }
  }

  return true;
}

#if NOSTR_USE_CUSTOM_STRING_FUNCS
#define strncmp nostr_strncmp
#endif

static inline bool strncmp_sensitive(
  const char*  str1,
  const char*  str2,
  const size_t str1capacity,
  const size_t str2capacity,
  const bool   case_sensitive)
{
  require_not_null(str1, false);
  require_not_null(str2, false);
  require_valid_length(str1capacity, false);
  require_valid_length(str2capacity, false);

  size_t capacity = (str1capacity > str2capacity) ? str2capacity : str1capacity;
  if (capacity == 0) {
    return false;
  }

  typedef bool (*PCompareFunc)(const char a, const char b);
  PCompareFunc compare_func = (case_sensitive) ? chrcmp_sensitive : chrcmp;

  for (size_t i = 0; i < capacity; i++) {
    if (!compare_func(str1[i], str2[i])) {
      return false;
    }
  }

  return true;
}

static inline int32_t strpos_sensitive(
  const char*  base,
  const size_t base_len,
  const char*  target,
  const size_t target_len,
  const bool   case_sensitive)
{
  require_not_null(base, -1);
  require_not_null(target, -1);
  require_valid_length(base_len, -1);
  require_valid_length(target_len, -1);

  if (target_len > base_len) {
    return -1;
  }

  for (size_t base_pos = 0; base_pos <= (base_len - target_len); base_pos++) {
    if (strncmp_sensitive(&base[base_pos], target, base_len, target_len, case_sensitive)) {
      return base_pos;
    }
  }

  return -1;
}

static inline bool strstr_sensitive(
  const char*  base,
  const size_t base_len,
  const char*  target,
  const size_t target_len,
  const bool   case_sensitive)
{
  return (strpos_sensitive(base, base_len, target, target_len, case_sensitive) != -1);
}

static inline int32_t skip_space(const char* buffer, const size_t buffer_size)
{
  require_not_null(buffer, -1);
  require_valid_length(buffer_size, -1);

  size_t pos = 0;
  while (!is_empty(&buffer[pos]) && pos < buffer_size) {
    char current = value(&buffer[pos]);
    if (is_space(current)) {
      pos++;
      continue;
    }

    if (pos + 2 < buffer_size) {
      if (is_utf8_space(&buffer[pos])) {
        pos += 3;
        continue;
      }
    }

    return pos;
  }

  return pos;
}

static inline int32_t skip_word(const char* buffer, const size_t buffer_size)
{
  for (size_t i = 0; i < buffer_size; i++) {
    if (is_space(buffer[i])) {
      return i;
    }

    if (is_empty_value(buffer[i])) {
      return i;
    }

    if (i + 2 >= buffer_size) {
      continue;
    }

    if (is_utf8_space(&buffer[i])) {
      return i;
    }
  }

  return buffer_size;
}

static inline int32_t skip_next_line(const char* buffer, const size_t buffer_len)
{
  require_not_null(buffer, 0);
  require_valid_length(buffer_len - 2, 0);

  size_t buffer_pos = 0;
  while (buffer[buffer_pos] != '\0' && buffer_pos < buffer_len - 1) {
    char current = buffer[buffer_pos];
    char next    = buffer[buffer_pos + 1];

    if (current == '\n') {
      return buffer_pos + 1;
    }

    if (next == '\n') {
      return buffer_pos + 2;
    }

    buffer_pos++;
  }

  return buffer_pos;
}

static inline int32_t skip_token(const char* buffer, const size_t buffer_size, const char token)
{
  require_not_null(buffer, -1);
  require_valid_length(buffer_size, -1);

  for (size_t i = 0; i < buffer_size; i++) {
    if (buffer[i] == token) {
      return i;
    }
  }

  return -1;
}

static size_t inline nostr_strlen(const char* str)
{
  require_not_null(str, 0);

  int32_t len = 0;
  while (str[len++] != '\0')
    ;

  return len - 1;
}

static size_t inline nostr_strnlen(const char* str, const size_t capacity)
{
  require_not_null(str, 0);
  require_valid_length(capacity, 0);

  int32_t len = 0;
  while (str[len++] != '\0' && len < capacity)
    ;

  return len - 1;
}

#if NOSTR_USE_CUSTOM_STRING_FUNCS
#define strlen nostr_strlen
#define strnlen nostr_strnlen
#endif

static inline int32_t calc_digit(int32_t value)
{
  int32_t is_negative = (value < 0);
  if (is_negative) {
    value = -value;
  }

  for (int32_t i = 10, j = 1; i < 1000000000; i *= 10, j++) {
    if (value < i) {
      return j;
    }
  }

  return 10 + is_negative;
}

static inline size_t itoa(int32_t value, char* buffer, size_t buffer_capacity)
{
  require_not_null(buffer, 0);
  require_valid_length(buffer_capacity, 0);

  int32_t digit       = calc_digit(value);
  char*   end         = buffer + digit;
  char*   current     = end;
  int32_t is_negative = (value < 0);

  if (is_negative) {
    value = -value;
  }

  *current-- = '\0';
  do {
    *current-- = '0' + (value % 10);
    value /= 10;
  } while (value > 0 && current >= buffer);

  if (is_negative && current >= buffer) {
    *current-- = '-';
  }
  return end - (current + 1);
}

#endif
