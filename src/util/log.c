#include "log.h"

#include "../arch/write.h"
#include "./string.h"

static inline void hex_dump_char(char c)
{
  unsigned char uc           = (unsigned char)c;
  char          hex_digits[] = "0123456789ABCDEF";
  char          buf[3];

  buf[0] = hex_digits[uc / 16];
  buf[1] = hex_digits[uc % 16];
  buf[2] = ' ';

  internal_write(STDOUT_FILENO, buf, 3);
}

bool hex_dump_local(const void* restrict data, size_t size)
{
  require_not_null(data, false);
  require_valid_length(size, false);

  const char* byte_data = (const char*)data;

  for (size_t i = 0; i < size; i++) {
    hex_dump_char(byte_data[i]);

    if ((i + 1) % 16 == 0) {
      internal_write(STDOUT_FILENO, "\n", 1);
    }
  }

  if (size % 16 != 0) {
    internal_write(STDOUT_FILENO, "\n", 1);
  }

  return true;
}

bool log_dump_local(const int32_t fd, const char* restrict str, const char* file, int line)
{
  require_valid_length(fd, false);
  require_not_null(str, false);
  require_not_null(file, false);
  require_valid_length(line, false);
  size_t len = strlen(str);
  require_valid_length(len, false);

#ifdef LOG_LEVE_DEBUG
  char   linestr[8];
  size_t linestr_size   = itoa(line, linestr, sizeof(linestr));
  linestr[linestr_size] = '\0';

  (void)internal_write(fd, "[", 1);
  (void)internal_write(fd, file, strlen(file));
  (void)internal_write(fd, ":", 1);
  (void)internal_write(fd, linestr, strlen(linestr));
  (void)internal_write(fd, "] ", 2);
#endif
  (void)internal_write(fd, str, len);
}

bool var_dump_local(const int32_t fd, const char* restrict str, const int32_t value)
{
  require_valid_length(fd, false);
  require_not_null(str, false);
  size_t len = strlen(str);
  require_valid_length(len, false);

  (void)internal_write(fd, str, len);

  char   buffer[32];
  size_t buffer_size      = itoa(value, buffer, sizeof(buffer));
  buffer[buffer_size]     = '\n';
  buffer[buffer_size + 1] = '\0';

  return (internal_write(fd, buffer, buffer_size + 1) > 0);
}

bool str_dump_local(const int32_t fd, const char* restrict str, const char* restrict value)
{
  require_valid_length(fd, false);
  require_not_null(str, false);
  require_not_null(value, false);

  log_dump(fd, str);
  log_dump(fd, value);
  log_dump(fd, "\n");

  return true;
}
