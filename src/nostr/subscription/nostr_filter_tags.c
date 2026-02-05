#include "../../arch/memory.h"
#include "../../util/log.h"
#include "../../util/string.h"
#include "nostr_filter.h"

// ============================================================================
// Helper: Convert hex char to value
// ============================================================================
static int32_t hex_char_value(char c)
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
// Helper: Convert hex string to bytes
// Returns number of bytes written
// ============================================================================
static size_t hex_to_bytes(const char* hex, size_t hex_len, uint8_t* out, size_t out_capacity)
{
  size_t pairs = hex_len / 2;
  if (pairs > out_capacity) {
    pairs = out_capacity;
  }

  for (size_t i = 0; i < pairs; i++) {
    int32_t h = hex_char_value(hex[i * 2]);
    int32_t l = hex_char_value(hex[i * 2 + 1]);

    if (h < 0 || l < 0) {
      return i;
    }

    out[i] = (uint8_t)((h << 4) | l);
  }

  return pairs;
}

// ============================================================================
// Helper: Check if string is valid hex
// ============================================================================
static bool is_valid_hex(const char* str, size_t len)
{
  for (size_t i = 0; i < len; i++) {
    if (hex_char_value(str[i]) < 0) {
      return false;
    }
  }
  return true;
}

bool extract_nostr_filter_tag(
  const PJsonFuncs funcs,
  const char*      json,
  const jsontok_t* token,
  char             tag_name,
  NostrFilterTag*  tag)
{
  require_not_null(funcs, false);
  require_not_null(json, false);
  require_not_null(token, false);
  require_not_null(tag, false);

  if (!funcs->is_array(token)) {
    log_debug("Filter error: tag array is not an array\n");
    return false;
  }

  int num_values = token->size;
  if (num_values < 0) {
    log_debug("Filter error: invalid tag array size\n");
    return false;
  }

  // Initialize
  internal_memset(tag, 0, sizeof(NostrFilterTag));
  tag->name         = tag_name;
  tag->values_count = 0;

  if (num_values == 0) {
    return true;
  }

  int token_idx = 1;

  for (int i = 0; i < num_values && tag->values_count < NOSTR_FILTER_MAX_TAG_VALUES; i++) {
    const jsontok_t* val_token = &token[token_idx];

    if (!funcs->is_string(val_token)) {
      log_debug("Filter error: tag value is not a string\n");
      token_idx++;
      continue;
    }

    size_t val_len = funcs->get_token_length(val_token);
    if (val_len == 0) {
      token_idx++;
      continue;
    }

    if (val_len > 64) {
      token_idx++;
      continue;
    }

    const char* val_str = &json[val_token->start];

    // For 'e' and 'p' tags, values should be 64-char hex (32 bytes binary)
    // For other tags like 't', they can be arbitrary strings
    if (tag_name == 'e' || tag_name == 'p') {
      // Validate hex and convert to binary
      if (val_len == 64 && is_valid_hex(val_str, val_len)) {
        hex_to_bytes(val_str, val_len, tag->values[tag->values_count], 32);
        tag->values_count++;
      }
    } else {
      // For generic tags (like #t), store as-is (up to 32 bytes)
      size_t copy_len = val_len < 32 ? val_len : 32;
      internal_memcpy(tag->values[tag->values_count], val_str, copy_len);
      tag->values_count++;
    }

    token_idx++;
  }

  return true;
}
