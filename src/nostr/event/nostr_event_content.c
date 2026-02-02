#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "../nostr_func.h"

// ============================================================================
// Helper: Convert hex digit to value
// ============================================================================
static int32_t hex_digit_to_value(char c)
{
  if (is_digit(c)) {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

// ============================================================================
// Helper: Decode JSON escape sequences
// Returns the number of bytes written to output, or -1 on error
// ============================================================================
static int64_t decode_json_string(
  const char* src,
  size_t      src_len,
  char*       dst,
  size_t      dst_capacity)
{
  if (is_null(src) || is_null(dst) || dst_capacity == 0) {
    return -1;
  }

  size_t src_idx = 0;
  size_t dst_idx = 0;

  while (src_idx < src_len && dst_idx < dst_capacity - 1) {
    char c = src[src_idx];

    if (c == '\\' && src_idx + 1 < src_len) {
      // Escape sequence
      char next = src[src_idx + 1];
      src_idx += 2;

      switch (next) {
        case '"':
          dst[dst_idx++] = '"';
          break;
        case '\\':
          dst[dst_idx++] = '\\';
          break;
        case '/':
          dst[dst_idx++] = '/';
          break;
        case 'b':
          dst[dst_idx++] = '\b';
          break;
        case 'f':
          dst[dst_idx++] = '\f';
          break;
        case 'n':
          dst[dst_idx++] = '\n';
          break;
        case 'r':
          dst[dst_idx++] = '\r';
          break;
        case 't':
          dst[dst_idx++] = '\t';
          break;
        case 'u':
          // Unicode escape: \uXXXX
          if (src_idx + 4 <= src_len) {
            int32_t h1 = hex_digit_to_value(src[src_idx]);
            int32_t h2 = hex_digit_to_value(src[src_idx + 1]);
            int32_t h3 = hex_digit_to_value(src[src_idx + 2]);
            int32_t h4 = hex_digit_to_value(src[src_idx + 3]);

            if (h1 < 0 || h2 < 0 || h3 < 0 || h4 < 0) {
              // Invalid hex digits, copy as-is
              dst[dst_idx++] = '\\';
              dst[dst_idx++] = 'u';
            } else {
              uint32_t codepoint = (uint32_t)((h1 << 12) | (h2 << 8) | (h3 << 4) | h4);
              src_idx += 4;

              // Encode as UTF-8
              if (codepoint < 0x80) {
                dst[dst_idx++] = (char)codepoint;
              } else if (codepoint < 0x800) {
                if (dst_idx + 2 > dst_capacity - 1) break;
                dst[dst_idx++] = (char)(0xC0 | (codepoint >> 6));
                dst[dst_idx++] = (char)(0x80 | (codepoint & 0x3F));
              } else {
                if (dst_idx + 3 > dst_capacity - 1) break;
                dst[dst_idx++] = (char)(0xE0 | (codepoint >> 12));
                dst[dst_idx++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                dst[dst_idx++] = (char)(0x80 | (codepoint & 0x3F));
              }
            }
          } else {
            // Not enough characters for \uXXXX
            dst[dst_idx++] = '\\';
            dst[dst_idx++] = 'u';
          }
          break;
        default:
          // Unknown escape, keep as-is
          dst[dst_idx++] = '\\';
          if (dst_idx < dst_capacity - 1) {
            dst[dst_idx++] = next;
          }
          break;
      }
    } else {
      // Regular character
      dst[dst_idx++] = c;
      src_idx++;
    }
  }

  dst[dst_idx] = '\0';
  return (int64_t)dst_idx;
}

// ============================================================================
// extract_nostr_event_content
// ============================================================================
bool extract_nostr_event_content(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char*            content,
  size_t           content_capacity)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(content, false);

  if (!funcs->is_string(token)) {
    log_debug("Nostr Event Error: content is not string\n");
    return false;
  }

  size_t content_len = funcs->get_token_length(token);

  // Check capacity (need room for null terminator)
  if (content_len >= content_capacity) {
    log_debug("Nostr Event Error: content too long\n");
    return false;
  }

  // Decode JSON string (handle escape sequences)
  int64_t decoded_len = decode_json_string(
    &json[token->start],
    content_len,
    content,
    content_capacity);

  if (decoded_len < 0) {
    log_debug("Nostr Event Error: failed to decode content\n");
    return false;
  }

  return true;
}
